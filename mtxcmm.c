/* vim:set ts=8 sw=4 et: */
/*
MDVIEW MTX

Copyright (C) 2024 step, https://github.com/step-

Licensed under the GNU General Public License Version 2

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
/*
MDVIEW MTX is a continuation of the mdview3 project.

mdview3 -- http://chiselapp.com/user/jamesbond/repository/mdview3
Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
Copyright (C) 2009 Leandro Pereira <leandro@hardinfo.org>
Copyright (C) 2015 James B
Copyright (C) 2016, 2023 step
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_58
#include <glib.h>
#include <libintl.h>
#include <ctype.h>
#include <pango/pango.h>
#include <pango/pango-utils.h>

#include "mtx.h"
#include "mtxcmm.h"
#include "mtxcmmprivate.h"
#include "mtxstylepango.h"
#include "mtxdbg.h"

struct _MtxCmmPrivate
{
    gboolean           escape;
    gboolean           escaping;         /* cached because frequently used */
    gint               ctr_repl_eval;    /* mtx_cmm_string_release_protected */
    MtxCmmParserUnitType seen_unit_types;  /* by mtx_cmm_render */
    gboolean           inside_table;     /* in <table> <a> selector */

    /* With public getters and setters */
    MtxCmmExtensions   extensions;
    MtxCmmTweaks       tweaks;
    MtxCmmOutput       output;
    MtxCmmTags         tags;            /* viewer sets, renderer gets */

    /* Parser queues. */
    GQueue             *unitq;          /* parsed markdown */
    gpointer           *unitq_head;     /* cache top element */
    GQueue             *junkq;          /* parser scraps */

    /* Parser work-tables */
    GArray             *regex_table;    /* precompiled regex */
    GPtrArray          *link_table;     /* URL/image and attributes */
    GPtrArray          *code_table;     /* <code>, protect sundries */
};

/**********************************************************************/


/**********************************************************************/

G_DEFINE_TYPE_WITH_CODE (MtxCmm, mtx_cmm, G_TYPE_OBJECT, G_ADD_PRIVATE (MtxCmm))

/**********************************************************************/

/*< private >**********************************************************/

static void mtx_cmm_parser_clear_queues (MtxCmm *);
static void mtx_cmm_parser_clear_regex_table_el (GRegex **e);

static void
mtx_cmm_finalize (GObject *object)
{
    MtxCmm *self;

    g_return_if_fail (MTX_IS_CMM (object));

    self = MTX_CMM (object);

    g_return_if_fail (self->priv != NULL);

    /* all three with GDestroyNotify function */
    g_array_free (self->priv->regex_table, TRUE);
    g_ptr_array_free (self->priv->link_table,  TRUE);
    g_ptr_array_free (self->priv->code_table,  TRUE);

    mtx_cmm_parser_clear_queues (self);
    g_clear_pointer (&self->priv->unitq, g_queue_free);  /* NOLINT(bugprone-sizeof-expression) */
    g_clear_pointer (&self->priv->junkq, g_queue_free);  /* NOLINT(bugprone-sizeof-expression) */

    G_OBJECT_CLASS (mtx_cmm_parent_class)->finalize (object);
}

static void
mtx_cmm_class_init (MtxCmmClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = mtx_cmm_finalize;
    g_type_ensure (MTX_TYPE_CMM);
}

static void
mtx_cmm_init (MtxCmm *self)
{
    self->priv = mtx_cmm_get_instance_private (self);
    self->priv->extensions = MTX_CMM_EXTENSION_NONE;
    self->priv->tweaks = MTX_CMM_TWEAK_NONE;
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
    self->priv->tweaks &= ~MTX_CMM_TWEAK_CM_BLOCK_END;
#else
    self->priv->tweaks |= MTX_CMM_TWEAK_CM_BLOCK_END;
#endif
    self->priv->output = MTX_CMM_OUTPUT_UNKNOWN;
    self->priv->escape = FALSE;
    self->priv->unitq = g_queue_new ();
    self->priv->junkq = g_queue_new ();

    self->priv->regex_table = g_array_set_size(g_array_sized_new(FALSE, TRUE, sizeof (GRegex *), MTX_CMM_REGEX_LEN), MTX_CMM_REGEX_LEN);
    g_array_set_clear_func (self->priv->regex_table, (GDestroyNotify) mtx_cmm_parser_clear_regex_table_el);

    self->priv->link_table = g_ptr_array_new_with_free_func (g_free);
    self->priv->code_table = g_ptr_array_new_with_free_func (g_free);
}

/*< private >********************************************************/

/*********************************************************************
*                           PARSER TABLES                            *
*********************************************************************/

static void
mtx_cmm_parser_clear_regex_table_el (GRegex **e)
{
    if (*e)
    {
        g_regex_unref (*e);
        *e = NULL;
    }
}

/*********************************************************************
*                         PARSER QUEUES                              *
*********************************************************************/

/**
mtx_cmm_parser_unit_cache_head:
Cache and get the current head.

Returns: the current head.
*/
static inline MtxCmmParserUnit *
mtx_cmm_parser_unit_cache_head (MtxCmm *self)
{
    return (MtxCmmParserUnit *) (self->priv->unitq_head =
                                 g_queue_peek_head (self->priv->unitq));
}

/**
mtx_cmm_parser_unit_pop_head:
Pop the current parser queue head, and track the new head.
Never free a popped unit. Call mtx_cmm_parser_unit_consume() instead.

Returns: the popped head.
*/
static inline MtxCmmParserUnit *
mtx_cmm_parser_unit_pop_head (MtxCmm *self)
{
    MtxCmmParserUnit* head = g_queue_pop_head (self->priv->unitq);
    g_queue_push_tail (self->priv->junkq, head);
    mtx_cmm_parser_unit_cache_head (self);
    return head;
}

/**
mtx_cmm_parser_unit_free_arg:
@argptr: address of a unit argument's character array.

Free the argument.
*/
static void
mtx_cmm_parser_unit_free_arg (gchar **argptr)
{
    g_free (*argptr);
}

/**
mtx_cmm_parser_unit_consume:

@unitptr: address of pointer to a %MtxCmmParserUnit.

Never fully remove a unit from its queue. Call this function instead.
*/
static inline void
mtx_cmm_parser_unit_consume (MtxCmmParserUnit **unitptr)
{
    (*unitptr)->type = MTX_CMM_PARSER_UNIT_JUNK;
}

/**
mtx_cmm_parser_unit_free:

@self: %MtxCmm instance.
@unit: %MtxCmmParserUnit.
*/
static void
mtx_cmm_parser_unit_free (MtxCmm *self,
                          MtxCmmParserUnit *unit)
{
    if (unit->text)
    {
        g_string_free (unit->text, TRUE);
        unit->text = NULL;
    }
    if (unit->args)
    {
        g_array_free (unit->args, TRUE);
        unit->args = NULL;
    }
    if (unit == (MtxCmmParserUnit *) self->priv->unitq_head)
    {
        self->priv->unitq_head = NULL;
    }
    g_free (unit);
}

/**
mtx_cmm_parser_unit_clear:

@unit: %MtxCmmParserUnit.
@self: %MtxCmm instance.

Redirected to %mtx_cmm_parser_unit_free.
*/
static inline void
mtx_cmm_parser_unit_clear (MtxCmmParserUnit *unit,
                           MtxCmm *self)
{
    mtx_cmm_parser_unit_free (self, unit);
}

/**
mtx_cmm_parser_clear_queues:

@self: %MtxCmm instance.

Clear parser unit queues leaving GQueue objects allocated.
*/

static void
mtx_cmm_parser_clear_queues (MtxCmm *self)
{
    if (self->priv->unitq != NULL)
    {
        g_queue_foreach (self->priv->unitq, (GFunc) mtx_cmm_parser_unit_clear,
                         self);
        g_queue_clear (self->priv->unitq);
    }
    self->priv->unitq_head = NULL;
    if (self->priv->junkq != NULL)
    {
        g_queue_foreach (self->priv->junkq, (GFunc) mtx_cmm_parser_unit_clear,
                         self);
        g_queue_clear (self->priv->junkq);
    }
}

/*< public  >********************************************************/

/**
mtx_cmm_parser_unit_new:
Create a new unit and push it on the private parser queue.

@self: %MtxCmm instance.
@type: %MtxCmmParserUnitType.
@flag_mask: %MtxCmmParserUnitFlag bit mask.

Freeing the new unit with mtx_cmm_parser_unit_free is generally unnecessary as
mtx_cmm_parser_clear_queues() clears parser queues when parsing (re)starts, and
when @self is finalized.
*/
/*static*/ void
mtx_cmm_parser_unit_new (MtxCmm *self,
                         const MtxCmmParserUnitType type,
                         const MtxCmmParserUnitFlag flag_mask)
{
    MtxCmmParserUnit *head;

    head = (MtxCmmParserUnit *) (self->priv->unitq_head =
                                 g_malloc0 (sizeof (MtxCmmParserUnit)));
    g_queue_push_head (self->priv->unitq, head);
    head->type = type;
    head->flag = flag_mask;
    if (head->flag & MTX_CMM_PARSER_UNIT_FLAG_ARGS)
    {
        head->args = g_array_new (TRUE, FALSE, sizeof (gchar *));
        g_array_set_clear_func (head->args,
                                (GDestroyNotify) mtx_cmm_parser_unit_free_arg);
    }
}

/**
mtx_cmm_new:

Create a new %MTX_CMM instance.

Returns: the new instance.
*/
MtxCmm *
mtx_cmm_new (void)
{
    MtxCmm *self = g_object_new (MTX_TYPE_CMM, NULL);
    return MTX_CMM (self);
}

static const gchar * const _tag_info[] = {
    [MTX_TAG_DEST_LINK_URI_ID]   = "dest=Lu",
    [MTX_TAG_DEST_LINK_TXT_LEN]  = "dest=Tl",
    [MTX_TAG_DEST_IMAGE_PATH_ID] = "dest=Ip",
    [MTX_TAG_BLOCKQUOTE_LEVEL]   = "blckqtLvl=",
    [MTX_TAG_BLOCKQUOTE_OPEN]    = "blckqtOpn=",
    [MTX_TAG_OL_UL_LEVEL]        = "olUlLvl=",
    [MTX_TAG_LI_LEVEL]           = "liLvl=",
    [MTX_TAG_LI_ORDINAL]         = "liOrd=",
    [MTX_TAG_LI_BULLET_LEN]      = "liBLen=",
    [MTX_TAG_LI_ID]              = "liId=",
};

/**
mtx_cmm_tag_get_info:

@tag: pango "font" tag
@subject: MtxCmmTagInfo
Return: `gint` @subject's value parsed from @tag
*/
gint
mtx_cmm_tag_get_info (MtxCmm *self,
                      const gchar *tag,
                      const MtxCmmTagInfo subject)
{
    gchar *p = NULL;
    gint ret = -1;

    g_return_val_if_fail (self != NULL, -1);
    g_return_val_if_fail (tag != NULL, -1);
    g_return_val_if_fail (subject < MTX_TAG_INFO_LEN, -1);
    p = strstr (tag, _tag_info[subject]);
    if (p)
    {
        ret = atoi (p + strlen (_tag_info[subject]));
    }
    return ret;
}

#if MTX_DEBUG > 1
/* standout, standout end */
#define _SO    "\033[7m"
#define _SE    "\033[0m"
#define _SObla "\033[7;30m"
#define _SOred "\033[7;31m"
#define _SOgre "\033[7;32m"
#define _SOyel "\033[7;33;46m"
#define _SOblu "\033[7;34m"
#define _SOmag "\033[7;35m"
#define _SOcya "\033[7;36m"
#define _SOwhi "\033[7;37m"

static void
_print_priv_table (const GPtrArray *priv_table,
                   const gchar *fmt,
                   ...)
{
    va_list args;
    va_start (args, fmt);
    gchar *msg = g_strdup_vprintf (fmt, args);
    va_end (args);
    g_printerr ("%s", msg);
    g_free (msg);
    if (priv_table->len == 0)
    {
        g_printerr (_SO "EMPTY TABLE" _SE "\n");
    }
    else
    {
        for (guint i = 0; i < priv_table->len; i++)
        {
            gchar *p = g_ptr_array_index (priv_table, i);
            g_printerr ("% 3d. \"%s\"\n", i, p);
        }
    }
}

#endif /* MTX_DEBUG > 1 */

/**
mtx_cmm_make_code_ref:

@id: id of an inline code segment stashed with `mtx_cmm_stash_code`.
Return:
dynamically-allocated string code_ref, which encodes @id
*/
static inline gchar*
mtx_cmm_make_code_ref (const guint id)
{
    return g_strdup_printf ("%s%dC;%s", sUNIPUA_CODE, id, sUNIPUA_CODE);
}

/**
mtx_cmm_get_code_id:

@ref    code_ref text
Return: -1   if @text isn't a valid code_ref otherwise return the code id
*/
static inline gint
mtx_cmm_get_code_id (
    const gchar *ref)
{
    gint id = -1;
    if (strncmp (ref, sUNIPUA_CODE, sizeof (sUNIPUA_CODE) - 1) == 0)
    {
        id = atoi (ref + sizeof (sUNIPUA_CODE) - 1);
    }

    return id;
}

/**
mtx_cmm_stash_code:
@code: markdown code span or `code_ref`
Return:
stashed `code_id` - use `mtx_cmm_get_code` to retrieve the code text.
Line endings are converted to space according to the CM spec.
Identical @codes are stashed with the same id.
*/
static inline guint
mtx_cmm_stash_code (MtxCmm *self,
                    const gchar *code)
{
    gint id = mtx_cmm_get_code_id (code);
    if (id < 0)
    {
        gchar *p;
        for (guint i = 0; i < self->priv->code_table->len; i++)
        {
            p = g_ptr_array_index (self->priv->code_table, i);
            if (strcmp (code, p) == 0)
            {
                id = i;
                break;
            }
        }
        if (id < 0)
        {
            p = g_strdup (code);
            g_ptr_array_add (self->priv->code_table, p);
            id = self->priv->code_table->len - 1;
        }
    }

    return id;
}

/**
mtx_cmm_get_code:
@id: id of a code span previously stashed with `mtx_cmm_stash_code`.
Return: code text containing unescaped grave accents. The instance owns the
returned memory.
*/
static gchar *
mtx_cmm_get_code (MtxCmm *self,
                  const gint id)
{
    g_return_val_if_fail(id < (gint) self->priv->code_table->len, NULL);
    return g_ptr_array_index (self->priv->code_table, id);
}

/**
mtx_cmm_regex_code_ref:

Return: a GRegex that matches the internal code_refs created by
mtx_cmm_make_code_ref.
*/
static GRegex *
mtx_cmm_regex_code_ref (MtxCmm *self)
{
    static GRegex **regex = NULL;

    if (regex == NULL)
    {
        regex =
         &g_array_index (self->priv->regex_table, GRegex *,
                         MTX_CMM_REGEX_CODE_REF);
    }
    if (*regex == NULL)
    {
        *regex = g_regex_new (sUNIPUA_CODE "(\\d+)C;" sUNIPUA_CODE, 0, 0, NULL);
    }
    return *regex;
}

/**
mtx_cmm_get_link_dest_id:

@ref: a `link_dest_ref`

Return: the link DEST id otherwise -1 if @ref is invalid.
*/
static inline gint
mtx_cmm_get_link_dest_id (const gchar *ref)
{
    gint id = -1;
    if (strncmp (ref, sUNIPUA_LINK, sizeof (sUNIPUA_LINK) - 1) == 0)
    {
        id = atoi (ref + sizeof (sUNIPUA_LINK) - 1);
    }
    return id;
}

/**
mtx_cmm_stash_link_dest:

@dest: markdown link destination or `link_dest_ref`

Return: stashed `link_dest_id`. Use %mtx_cmm_get_link_dest to retrieve the link
destination.  Identical destinations are stashed with the same id.

This function also applies to markdown image path.
*/
static gint
mtx_cmm_stash_link_dest (MtxCmm *self,
                         const gchar *dest)
{
    gint id = mtx_cmm_get_link_dest_id (dest);
    if (id < 0)
    {
        gchar *p;
        for (guint i = 0; i < self->priv->link_table->len; i++)
        {
            p = g_ptr_array_index (self->priv->link_table, i);
            if (strcmp (dest, p) == 0)
            {
                id = i;
                break;
            }
        }
        if (id < 0)
        {
            p = g_strdup (dest);
            g_ptr_array_add (self->priv->link_table, p);
            id = self->priv->link_table->len - 1;
        }
    }
    return id;
}

/**
mtx_cmm_get_link_dest:

@id: link destination id in @self's link_table.

Return: pointer to link URI or image path strings owned by the instance.
*/
const gchar *
mtx_cmm_get_link_dest (MtxCmm *self,
                       const gint id)
{
    g_return_val_if_fail (id < (gint) self->priv->link_table->len, NULL);
    return (const gchar *) g_ptr_array_index(self->priv->link_table, id);
}

/**
mtx_cmm_mtx_reset:
*/
static void
mtx_cmm_mtx_reset (MtxCmm *self)
{
    gsize i, sz;
    gchar **a;

    a = (gchar **) g_ptr_array_steal (self->priv->link_table, &sz);
    for (i = 0; i < sz; i++)
    {
        g_free (a[i]);
    }
    g_free (a);

    a = (gchar **) g_ptr_array_steal (self->priv->code_table, &sz);
    for (i = 0; i < sz; i++)
    {
        g_free (a[i]);
    }
    g_free (a);

    mtx_cmm_parser_clear_queues (self);
    g_queue_free (self->priv->unitq);
    g_queue_free (self->priv->junkq);
    self->priv->unitq = g_queue_new ();
    self->priv->junkq = g_queue_new ();
}

/**
mtx_cmm_protect:
Stash text in @self->priv->code_table and return the corresponding `code_ref`.

@self:  the %MtxCmm instance.
@text:  the text to stash and protect.

Return: a `code_ref` string that can be used to replace @text in its original
location. The caller owns the memory.  Return NULL on error.

This function is used to encode text in opaque strings, hence preventing the
text from entangling with other text in subsequent parsing and rendering stages.
Use %mtx_cmm_string_release_protected to see the content of a a string containing
protected segments.
*/
static inline gchar *
mtx_cmm_protect (MtxCmm *self,
                 const gchar *text)
{
    gint id = mtx_cmm_stash_code (self, text);
    return id < 0 ? NULL : mtx_cmm_make_code_ref (id);
}

/**
mtx_replace_code_ref_cb:
Callback to replace a code_ref match with the referenced text.
*/
static gboolean
mtx_release_code_ref_cb (const GMatchInfo *match_info,
                         GString *buf,
                         gpointer data)
{
    MtxCmm *self = data;
    gchar *match = g_match_info_fetch (match_info, 1);
    gint id = atoi (match);
    gchar *code = mtx_cmm_get_code (self, id);
    g_string_append (buf, code);
    g_free (match);
    self->priv->ctr_repl_eval++;

    return FALSE;
}

/**
mtx_cmm_string_release_protected:
Replace all code_refs in @str with the text they reference.
@str: GString

Returns: the number of matches or -1 on error.
*/
static gint
mtx_cmm_string_release_protected (MtxCmm *self,
                                  GString *str)
{
    GRegex *regex = mtx_cmm_regex_code_ref (self);
    GError *err = NULL;

    self->priv->ctr_repl_eval = 0; /* NON-RE-ENTRANT */
    g_autofree gchar *temp =
    g_regex_replace_eval (regex, str->str, -1, 0, 0,
                          mtx_release_code_ref_cb, self, &err);
    if (err != NULL)
    {
        g_printerr ("%s: %s\n", __FUNCTION__, err->message);
        g_error_free (err);
        return -1;
    }
    g_string_assign (str, temp);
    return self->priv->ctr_repl_eval;
}

/**
mtx_strstrip_pango_markup:

Returns: TRUE and sets *@retptr to a newly-allocated buffer of plain text,
otherwise it returns FALSE for error.
*/
inline static gboolean
mtx_strstrip_pango_markup (const gchar *string,
                        gchar **retptr)
{
    PangoAttrList *attrs;
    gboolean ret =
    pango_parse_markup (string, -1, 0, &attrs, retptr, NULL, NULL);
    {
        pango_attr_list_unref (attrs); /* don't care */
    }
    return ret;
}

/**
mtx_strstrip_pango_spans_fast:
Naïve Pango <span> and <tt> tag stripper.

@string: a string to be modified in place.

This function does not allocate memory it changes @string directly and only
erases <span>, <tt> and their closing tags without semantic checking. Look at
`mtx_strstrip_pango_markup` for a complete and robust stripper.
*/
static void
mtx_strstrip_pango_spans_fast (gchar *string)
{
    gchar *p, *q, **t;
    const gchar *tags[] = {"<span ", "</span>", "<span>", "<tt>", "</tt>", 0};

    for (t = (gchar **) tags; *t != NULL; t++)
    {
        while ((p = strstr (string, *t)) != NULL)
        {
            if ((q = strchr (p, '>')) != NULL)
            {
                memmove (p, q + 1, strchr (q, '\0') - q);
            }
        }
    }
}

/**
mtx_cmm_string_release_protected_unmarked:
Recursively release all code_refs in @str then strip off Pango <span> markup
leaving only clear UTF-8 text, possibly containing UNIPUA codepoints, as the
result.

@str: GString

Returns: the number of matches or -1 on error. In either case @str->str
may have changed.
*/
static gint
mtx_cmm_string_release_protected_unmarked (MtxCmm *self,
                                           GString *str)
{
    gint ret, ctr = 0;

    do {
        ret = mtx_cmm_string_release_protected (self, str);
        if (ret > 0)
        {
            ctr += ret;
        }
    } while (ret > 0);
    if (ctr > 0)
    {
#if 0
        gchar *clear_text;
        (void) mtx_strstrip_pango_markup (str->str, &clear_text);
        g_string_assign (str, clear_text);
        g_free (clear_text);
#else
        /* not very robust but much faster; let's see it in practice... */
        mtx_strstrip_pango_spans_fast (str->str);
        str->len = strlen (str->str);
#endif
    }
    return ret < 0 ? -1 : ctr;
}

/**
mtx_cmm_linkbuilder_pango:

@text: can be NULL.
@dest: can be NULL, forwarded as font span.
@title: can be NULL.
@link_dest_id: can be -1.
@dest: the uri-encoded link followed by '\n' followed by the verbatim link.
*/
static gchar *
mtx_cmm_linkbuilder_pango (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id)
{
    /*
    Here and in mtx_cmm_imagebuilder_pango(), We need to pass extra data
    directly to the application that will render the link in the MTX TextView.
    Our data will piggyback the Pango markup "font" attribute. The TextView will
    receive the font string as a GtkTextTag, decode it to get the font attribute
    (string), decode the attribute and call mtx_cmm_get_link_dest() et al. to
    obtain the link DEST.

    The font attribute string looks like this:
      font="@dest=<type><id>[<type><value>...]",   with
      <type> "Lu"(URI), "Ip"(image path); <id> int(link table index aka id)
    The optional <type><value> continuation is only used for "Lu" to add
    the length of the link text. For example, markdown [texty](link) yields
      font="@dest=Lu1dest=Tl5"   assuming it's the first link in the document.

    The "@..." syntax is called font VARIATIONS, Pango supports it since version
    ??. VARIATIONS is a comma-separated list of font variation specifications
    of the form @axis=value (the = sign is optional). It's the last part of the
    font description{1}.  Is it safe to piggyback it? I tested <span>s using
    `pango-view --markup -t`, e.g.  font="Serif Bold Italic 44 @x=1,y=3,z=4",
    and played with "Serif", "Bold", "Italic", "44". It has always worked.

    CAVEAT: the added font property shadows other font properties set by outer
    spans. I suspect this is inherent in the GtkTextBuffer implementation: no
    font style inheritance of the kind we can enjoy with CSS.

    {1} https://docs.gtk.org/Pango/type_func.FontDescription.from_string.html
    {2} https://gitlab.gnome.org/GNOME/pango/-/blob/main/pango/fonts.c#L1252
        is the "[src]" link in {1}
    */
    gchar *ret = NULL, *esc_title = NULL;
    gchar *style = MTX_STYLE_PANGO_URL;
    GString *markup = NULL;
    guint llen;
    gchar *plain_text = NULL, *merge_img = NULL;

    /*
    The viewer app will retrieve the link URI from the link_table,
    and get: dest format: <uri-encoded>\n<verbatim>.
    Incoming 'text' is markdown link text, which can include pango markup now
    but will turn into plain text in the GtkTextBuffer. Therefore, on purpose,
    'llen' is the length of the link text after stripping markup, as it will
    appear to the viewer app.
    Said markup could include a markdown image (see examples/text-links-patch.md
    as to why).  If so, we need to merge the font description of the image into
    the font description of the link.
    */
    /*
    TODO: Support more than one image in link text. For now it's just 0 or 1.
    */

    if (text)
    {
        gint id = -1, repl_ctr = -1;
        gchar *p;

        mtx_dbg_errout (-1, "(%s)", text);
        /* Text could be encoded; we must decode to tell. */
        markup = g_string_new (text);
        repl_ctr = mtx_cmm_string_release_protected (self, markup);
        /* repl_ctr > 0 => text was encoded */
        mtx_dbg_errseq (-1, " ==> markup(%s)", markup->str);

        (void) mtx_strstrip_pango_markup (markup->str, &plain_text);
        mtx_dbg_errseq (-1, " ==> plain_text(%s)", plain_text);

        /*
        If text encoded (repl_ctr > 0) a markdown image (id >= 0),
        initialize a merge argument for the link font description.
        */
        if (repl_ctr > 0)
        {
            id = mtx_cmm_tag_get_info (self, markup->str,
                                       MTX_TAG_DEST_IMAGE_PATH_ID);
            if (id >= 0)
            {
                merge_img =
                g_strdup_printf ("@%s%d",
                                 _tag_info[MTX_TAG_DEST_IMAGE_PATH_ID], id);
                mtx_dbg_errseq (-1, " ==> merge_img(%s)", merge_img);
                p = g_strdup_printf ("font=\"%s\"", merge_img);
                g_string_replace (markup, p, "", 1);
                g_free (p);
                mtx_dbg_errseq (-1, " ==> markup(%s)", markup->str);
            }
        }
        g_string_assign (markup, p = mtx_cmm_protect (self, markup->str));
        g_free (p);
        mtx_dbg_errseq (-1, " ==> protected_markup(%s)", markup->str);
    }
    else
    {
        markup = g_string_new ("⯅⯅");
    }
    llen =
    plain_text != NULL ? (gsize) g_utf8_strlen (plain_text, -1) : markup->len;
    g_free (plain_text);
    mtx_dbg_errseq (-1, " [ markup %s ]", markup->str);
    mtx_dbg_errseq (-1, " [ llen %d ]\n", llen);

    if (title)
    {
        esc_title = g_markup_printf_escaped (" (%s)", title);
    }
    if (dest && link_dest_id >= 0)
    {
        ret =
        g_strdup_printf ("<span font=\"%s@%s%d%s%d%s\" %s>" "%s</span>%s",
                         self->priv->inside_table ? "monospace " : "",
                         _tag_info[MTX_TAG_DEST_LINK_URI_ID], link_dest_id,
                         _tag_info[MTX_TAG_DEST_LINK_TXT_LEN], llen,
                          merge_img != NULL ? merge_img : "",
                          style, markup->str, esc_title ? esc_title : "");
    }
    else  /* link w/o dest, e.g. [text]() is valid CommonMark. */
    {
        ret =
        g_strdup_printf ("<span %s%s>%s</span>%s",
                         self->priv->inside_table ? "font=\"monospace\"" : "",
                         style, markup->str, esc_title ? esc_title : "");
    }
    g_string_free (markup, TRUE);
    g_free (merge_img);
    g_free (esc_title);
    return ret;
}

/**
mtx_cmm_linkbuilder_html:

@text: can be NULL.
@dest: can be NULL.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded link followed by '\n' followed by the verbatim link.
*/
static gchar *
mtx_cmm_linkbuilder_html (MtxCmm *self __attribute__((unused)),
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id __attribute__((unused)))
{
    gchar *ret, *esc_title = NULL;

    /* dest format: <uri-encoded>\n<verbatim> */
    *strchr (dest, '\n') = '\0';

    if (title)
    {
        esc_title = g_markup_escape_text (title, -1);
    }
    if (esc_title)
    {
        ret = g_strdup_printf ("<a href=\"%s\" title=\"%s\">%s</a>", dest ?
                               dest : "", esc_title, text ? text : "");
    }
    else
    {
        ret = g_strdup_printf ("<a href=\"%s\">%s</a>", dest ? dest : "", text
                               ? text : "");
    }
    g_free (esc_title);
    return ret;
}

/**
mtx_cmm_linkbuilder_text:

@text: can be NULL.
@dest: can be NULL.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded link followed by '\n' followed by the verbatim link.
*/
static gchar *
mtx_cmm_linkbuilder_text (MtxCmm *self __attribute__((unused)),
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id __attribute__((unused)))
{
    gchar *p;
    /* dest format: <uri-encoded>\n<verbatim> */
    dest = strchr (dest, '\n') + 1;

    if (title)
    {
        p = g_strdup_printf ("%s%s%s%s%s%s(%s)", text ? text : "", text &&
                             dest ? " " : "",
                             dest ? "<" : "", dest ? dest : "", dest ? ">" :
                             "", text || dest ? " " : "", title);
    }
    else
    {
        p = g_strdup_printf ("%s%s%s%s%s", text ? text : "", text && dest ?
                             " " : "",
                             dest ? "<" : "", dest ? dest : "", dest ? ">" :
                             "");
    }
    return p;
}

/**
mtx_cmm_linkbuilder_ansi:

@text: can be NULL.
@dest: can be NULL.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded link followed by '\n' followed by the verbatim link.
*/
static gchar *
mtx_cmm_linkbuilder_ansi (MtxCmm *self __attribute__((unused)),
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id __attribute__((unused)))
{
    gchar *p;
    /* dest format: <uri-encoded>\n<verbatim> */
    dest = strchr (dest, '\n') + 1;

    if (title)
    {
        p = g_strdup_printf ("\033[1;93m%s" "\033[0m" "%s"
                             "\033[4;32m%s%s%s" "\033[0m" "%s"
                             "\033[37m(%s)" "\033[0m",
                             text ? text : "", text ? " " : "",
                             dest ? "<" : "", dest ? dest : "", dest ? ">"
                             : "", text || dest ? " " : "", title);
    }
    else
    {
        p = g_strdup_printf ("\033[1;93m%s" "\033[0m" "%s"
                             "\033[4;32m%s%s%s" "\033[0m",
                             text ? text : "", text ? " " : "",
                             dest ? "<" : "", dest ? dest : "", dest ? ">"
                             : "");
    }
    return p;
}

/**
mtx_cmm_imagebuilder_pango:
@text: can be NULL, image alternate text.
@dest: can be NULL, image path, ignored.
@title: can be NULL.
@link_dest_id: can be -1, forwarded as font span.
@dest: the uri-encoded image path followed by '\n' followed by the verbatim
image path.
 */
static gchar *
mtx_cmm_imagebuilder_pango (MtxCmm *self __attribute__((unused)),
                            const gchar *text,
                            const gchar *dest __attribute__((unused)),
                            const gchar *title,
                            const gint link_dest_id)
{
    gchar *ret, *esc_title = NULL;
    gchar *style = MTX_STYLE_PANGO_IMAGE;
    char alt[16] = { 0 };
    /* Similar comments here as in mtx_cmm_linkbuilder_pango. */

    /* dest format: <uri-encoded>\n<verbatim> */
    /* Send the pango viewer both formats.    */

    /*
    Text in a GtkTextBuffer can be marked with tags. A tag is an attribute that
    can be applied to some range of text. Here we create the tag with pango
    markup.  GtkTextView tags don't exist by themselves: they exist together
    with text != "".
     */

    /* For empty text, arbitrarily, fall back to the stringified link_dest_id */
    if (text == NULL)
    {
        snprintf (alt, sizeof alt, "%d", link_dest_id);
    }
    if (title)
    {
        esc_title = g_markup_printf_escaped (" (%s)", title);
    }
    ret =
    g_strdup_printf ("<span font=\"%s@%s%d\" %s>%s</span>%s",
                     self->priv->inside_table ? "monospace " : "",
                     _tag_info[MTX_TAG_DEST_IMAGE_PATH_ID],
                     link_dest_id, style, text ? text : alt, esc_title ?
                     esc_title : "");
    g_free (esc_title);
    return ret;
}

/**
mtx_cmm_imagebuilder_html:

@text: can be NULL, image alternate text.
@dest: can be NULL, image path.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded image path followed by '\n' followed by the verbatim
image path.
*/
static gchar *
mtx_cmm_imagebuilder_html (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id __attribute__((unused)))
{
    gchar *ret, *esc_text = NULL, *esc_title = NULL;
    gchar *angle_bracket;

    angle_bracket = self->priv->tweaks & MTX_CMM_TWEAK_HTML5 ? ">" : " />";

    /* dest format: <uri-encoded>\n<verbatim> */
    *strchr (dest, '\n') = '\0';

    if (text)
    {
        esc_text = g_markup_escape_text (text, -1);
    }
    if (title)
    {
        esc_title = g_markup_escape_text (title, -1);
    }
    if (esc_title)
    {
        ret = g_strdup_printf ("<img src=\"%s\" alt=\"%s\" title=\"%s\"%s",
                               dest ? dest : "", esc_text ? esc_text : "",
                               esc_title, angle_bracket);
    }
    else
    {
        ret = g_strdup_printf ("<img src=\"%s\" alt=\"%s\"%s", dest ? dest :
                               "", esc_text ? esc_text : "", angle_bracket);
    }
    g_free (esc_text);
    g_free (esc_title);
    return ret;
}

/**
mtx_cmm_imagebuilder_text:

@text: can be NULL, image alternate text.
@dest: can be NULL.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded image path followed by '\n' followed by the verbatim
image path.
*/
static gchar *
mtx_cmm_imagebuilder_text (MtxCmm *self __attribute__((unused)),
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id __attribute__((unused)))
{
    gchar *p;
    /* dest format: <uri-encoded>\n<verbatim> */
    dest = strchr (dest, '\n') + 1;

    if (title)
    {
        p = g_strdup_printf ("%s%s%s%s%s%s(%s)", text ? text : "", text ?
                             " " : "",
                             dest ? "{" : "", dest ? dest : "", dest ? "}"
                             : "", text || dest ? " " : "", title);
    }
    else
    {
        p = g_strdup_printf ("%s%s%s%s%s", text ? text : "", text ? " " :
                             "",
                             dest ? "<" : "", dest ? dest : "", dest ? ">"
                             : "");
    }
    return p;
}

/**
mtx_cmm_imagebuilder_ansi:

@text: can be NULL, image alternate text.
@dest: can be NULL.
@title: can be NULL.
@link_dest_id: ignored.
@dest: the uri-encoded image path followed by '\n' followed by the verbatim
image path.
*/
static gchar *
mtx_cmm_imagebuilder_ansi (MtxCmm *self __attribute__((unused)),
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id __attribute__((unused)))
{
    gchar *p;
    /* dest format: <uri-encoded>\n<verbatim> */
    dest = strchr (dest, '\n') + 1;
    if (title)
    {
        p = g_strdup_printf ("\033[1;93m%s" "\033[0m" "%s" "\033[4;32m%s%s%s"
                             "\033[0m" "%s" "\033[37m(%s)" "\033[0m",
                             text ? text : "", text ? " " : "",
                             dest ? "{" : "", dest ? dest : "",
                             dest ? "}" : "", text || dest ? " " : "", title);
    }
    else
    {
        p = g_strdup_printf ("\033[1;93m%s" "\033[0m" "%s" "\033[4;32m%s%s%s"
                             "\033[0m", text ? text : "", text ? " " : "",
                             dest ? "{" : "", dest ? dest : "",
                             dest ? "}" : "");
    }
    return p;
}

/**
mtx_cmm_get_render_indent:
*/
gboolean
mtx_cmm_get_render_indent (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    g_return_val_if_fail (self->priv->output != MTX_CMM_OUTPUT_UNKNOWN, FALSE);
    return (self->priv->output == MTX_CMM_OUTPUT_ANSI
            || self->priv->output == MTX_CMM_OUTPUT_TEXT
            || self->priv->output == MTX_CMM_OUTPUT_TTY);
}

/**
mtx_cmm_get_escape:
*/
gboolean
mtx_cmm_get_escape (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    return self->priv->escape;
}

/**
mtx_cmm_set_escape:
*/
gboolean
mtx_cmm_set_escape (MtxCmm *self,
                    gboolean escape)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    self->priv->escape = escape;
    return TRUE;
}

/**
mtx_cmm_get_extensions:

*/
MtxCmmExtensions
mtx_cmm_get_extensions (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    return self->priv->extensions;
}

/**
mtx_cmm_set_extensions:
*/
gboolean
mtx_cmm_set_extensions (MtxCmm *self,
                        const MtxCmmExtensions flags)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    self->priv->extensions = flags;
    return TRUE;
}

/**
mtx_cmm_get_tweaks:

*/
MtxCmmTweaks
mtx_cmm_get_tweaks (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    return self->priv->tweaks;
}

gboolean
mtx_cmm_set_tweaks (MtxCmm *self,
                    const MtxCmmTweaks flags)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    self->priv->tweaks = flags;
    return TRUE;
}

/**
mtx_cmm_get_output_tags:
*/
const MtxCmmTags *
mtx_cmm_get_output_tags (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), NULL);
    return (const MtxCmmTags *) &self->priv->tags;
}


MtxCmmOutput
mtx_cmm_get_output (MtxCmm *self)
{
    g_return_val_if_fail (MTX_IS_CMM (self), MTX_CMM_OUTPUT_UNKNOWN);
    return self->priv->output;
}

/**
mtx_cmm_set_output:
*/
gboolean
mtx_cmm_set_output (MtxCmm *self,
                    MtxCmmOutput output)
{ /* *INDENT*OFF* */
    gboolean ret = TRUE;
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);

    /* MTX PangoMarkup */
    if (output == MTX_CMM_OUTPUT_PANGO)
    {
        self->priv->tags.em_start = "<i>";
        self->priv->tags.em_end = "</i>";
        self->priv->tags.strong_start = "<b>";
        self->priv->tags.strong_end = "</b>";
        self->priv->tags.code_span_start =
            "<tt><span " MTX_STYLE_PANGO_CODE_SPAN ">";
        self->priv->tags.code_span_end = "</span></tt>";
        self->priv->tags.codeblock_start =
            "<tt><span " MTX_STYLE_PANGO_CODEBLOCK ">";
        self->priv->tags.codeblock_end = "</span></tt>\n";
        self->priv->tags.strikethrough_start = "<s>";
        self->priv->tags.strikethrough_end = "</s>";
        self->priv->tags.h1_start = "<b><span " MTX_STYLE_PANGO_H1 ">";
        self->priv->tags.h1_end = "</span></b>\n";
        self->priv->tags.h2_start = "<b><span " MTX_STYLE_PANGO_H2 ">";
        self->priv->tags.h2_end = "</span></b>\n";
        self->priv->tags.h3_start = "<b><span " MTX_STYLE_PANGO_H3 ">";
        self->priv->tags.h3_end = "</span></b>\n";
        self->priv->tags.h4_start = "<b><span " MTX_STYLE_PANGO_H4 ">";
        self->priv->tags.h4_end = "</span></b>\n";
        self->priv->tags.h5_start = "<b><span " MTX_STYLE_PANGO_H5 ">";
        self->priv->tags.h5_end = "</span></b>\n";
        self->priv->tags.h6_start = "<b><span " MTX_STYLE_PANGO_H6 ">";
        self->priv->tags.h6_end = "</span></b>\n";
        self->priv->tags.blockquote_start =      /* U+250C   NO newlines */
            "<span " MTX_STYLE_PANGO_BLOCKQUOTE ">┌   </span>";
        self->priv->tags.blockquote_end =        /* U+2514   NO newlines */
            "<span " MTX_STYLE_PANGO_BLOCKQUOTE ">└</span>";
        self->priv->tags.olist_start = "";
        self->priv->tags.olist_end = "";
        self->priv->tags.ulist_start = "";
        self->priv->tags.ulist_end = "";
        self->priv->tags.li_start[0] = "•";      /* render_open_li_block */
        self->priv->tags.li_start[1] = "◦";      /* render_open_li_block */
        self->priv->tags.li_end = "\n";          /* render_close_li_block */
        self->priv->tags.rule = "⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n";
        self->priv->tags.para_start = "";        /* render_open_p_block */
        self->priv->tags.para_end = "\n";        /* render_close_p_block */
        self->priv->tags.br = "\n";
        self->priv->tags.table_start = "<tt><span " MTX_STYLE_PANGO_TABLE ">";
        self->priv->tags.table_end = "</span></tt>\n";
        self->priv->tags.thead_start = "<tt><span " MTX_STYLE_PANGO_THEAD ">";
        self->priv->tags.thead_end = "</span></tt>";
        self->priv->tags.tbody_start = "";
        self->priv->tags.tbody_end = "";
        self->priv->tags.tr_start = "";
        self->priv->tags.tr_end = "│\n";
        self->priv->tags.th_start = "│<span " MTX_STYLE_PANGO_TH "> ";
        self->priv->tags.th_end = " </span>";
        self->priv->tags.td_start = "│<span " MTX_STYLE_PANGO_TD "> ";
        self->priv->tags.td_end = " </span>";
        self->priv->tags.link_builder = mtx_cmm_linkbuilder_pango;
        self->priv->tags.image_builder = mtx_cmm_imagebuilder_pango;

        /* MTX XHTML */
    }
    else if (output == MTX_CMM_OUTPUT_HTML)
    {
        self->priv->tags.em_start = "<em>";
        self->priv->tags.em_end = "</em>";
        self->priv->tags.strong_start = "<strong>";
        self->priv->tags.strong_end = "</strong>";
        self->priv->tags.code_span_start = "<code>";
        self->priv->tags.code_span_end = "</code>";
        self->priv->tags.codeblock_start =    /* render_open_code_block */
            "<pre><code";                     /* intentionally without '>' */
        self->priv->tags.codeblock_end = "</code></pre>\n";
        self->priv->tags.strikethrough_start = "<del>";
        self->priv->tags.strikethrough_end = "</del>";
        self->priv->tags.h1_start = "<h1>";
        self->priv->tags.h1_end = "</h1>\n";
        self->priv->tags.h2_start = "<h2>";
        self->priv->tags.h2_end = "</h2>\n";
        self->priv->tags.h3_start = "<h3>";
        self->priv->tags.h3_end = "</h3>\n";
        self->priv->tags.h4_start = "<h4>";
        self->priv->tags.h4_end = "</h4>\n";
        self->priv->tags.h5_start = "<h5>";
        self->priv->tags.h5_end = "</h5>\n";
        self->priv->tags.h6_start = "<h6>";
        self->priv->tags.h6_end = "</h6>\n";
        self->priv->tags.blockquote_start = "<blockquote>\n";
        self->priv->tags.blockquote_end = "</blockquote>\n";
        self->priv->tags.olist_start = "";        /* render_open_ol_block */
        self->priv->tags.olist_end = "</ol>\n";
        self->priv->tags.ulist_start = "<ul>\n";  /* render_open_ul_block */
        self->priv->tags.ulist_end = "</ul>\n";
        self->priv->tags.li_start[0] = "<li>";    /* render_open_li_block */
        self->priv->tags.li_start[1] = "<li>";    /* render_open_li_block */
        self->priv->tags.li_end = "</li>\n";      /* render_close_li_block */
        self->priv->tags.rule =                 /* render_open_hr_block */
            "<hr";                              /* intentionally without '>' */
        self->priv->tags.para_start = "<p>";    /* render_open_p_block */
        self->priv->tags.para_end = "</p>\n";   /* render_close_p_block */
        self->priv->tags.br =                   /* render_text_hardbr */
            "<br";                              /* intentionally without '>' */
        self->priv->tags.table_start = "<table>\n";
        self->priv->tags.table_end = "</table>\n";
        self->priv->tags.thead_start = "<thead>\n";
        self->priv->tags.thead_end = "</thead>\n";
        self->priv->tags.tbody_start = "<tbody>\n";
        self->priv->tags.tbody_end = "</tbody>\n";
        self->priv->tags.tr_start = "<tr>\n";
        self->priv->tags.tr_end = "</tr>\n";
        self->priv->tags.th_start = "<th%s>";
        self->priv->tags.th_end = "</th>\n";
        self->priv->tags.td_start = "<td%s>";
        self->priv->tags.td_end = "</td>\n";
        self->priv->tags.link_builder = mtx_cmm_linkbuilder_html;
        self->priv->tags.image_builder = mtx_cmm_imagebuilder_html;

        /* MTX tty - use Linux/vt100 codes */
    }
    else if (output == MTX_CMM_OUTPUT_TTY)
    {
        self->priv->tags.em_start = "\033[4m";
        self->priv->tags.em_end = "\033[0m";
        self->priv->tags.strong_start = "\033[1m";
        self->priv->tags.strong_end = "\033[0m";
        self->priv->tags.code_span_start = "\033[36m";
        self->priv->tags.code_span_end = "\033[39m";
        self->priv->tags.codeblock_start = "\033[36m";
        self->priv->tags.codeblock_end = "\033[39m\n";
        self->priv->tags.strikethrough_start = "--";
        self->priv->tags.strikethrough_end = "--";
        self->priv->tags.h1_start = "\033[7;1m";
        self->priv->tags.h1_end = "\033[27;21m\n";
        self->priv->tags.h2_start = "\033[7m";
        self->priv->tags.h2_end = "\033[27m\n";
        self->priv->tags.h3_start = "\033[1m";  /* same as bold */
        self->priv->tags.h3_end = "\033[21m\n";
        self->priv->tags.h4_start = "\033[1m";  /* same as bold */
        self->priv->tags.h4_end = "\033[21m\n";
        self->priv->tags.h5_start = "\033[1m";  /* same as bold */
        self->priv->tags.h5_end = "\033[21m\n";
        self->priv->tags.h6_start = "\033[1m";  /* same as bold */
        self->priv->tags.h6_end = "\033[21m\n";
        self->priv->tags.blockquote_start = "\033[1m>";
        self->priv->tags.blockquote_end = "\033[21m\n";
        self->priv->tags.olist_start = "";
        self->priv->tags.olist_end = "";
        self->priv->tags.ulist_start = "";
        self->priv->tags.ulist_end = "";
        self->priv->tags.li_start[0] = "*";     /* render_open_li_block */
        self->priv->tags.li_start[1] = "-";     /* render_open_li_block */
        self->priv->tags.li_end = "\n";         /* render_close_li_block */
        self->priv->tags.rule = " -----\n";
        self->priv->tags.para_start = "";       /* render_open_p_block */
        self->priv->tags.para_end = "\n";       /* render_close_p_block */
        self->priv->tags.br = "\n";
        self->priv->tags.table_start = "";
        self->priv->tags.table_end = "";
        self->priv->tags.thead_start = "";
        self->priv->tags.thead_end = "";
        self->priv->tags.tbody_start = "";
        self->priv->tags.tbody_end = "";
        self->priv->tags.tr_start = "";
        self->priv->tags.tr_end = "│\n";
        self->priv->tags.th_start = "│ ";
        self->priv->tags.th_end = " ";
        self->priv->tags.td_start = "│ ";
        self->priv->tags.td_end = " ";
        self->priv->tags.link_builder = mtx_cmm_linkbuilder_text;
        self->priv->tags.image_builder = mtx_cmm_imagebuilder_text;

        /* MTX ansi - use ANSI CSI codes imitating lowdown's */
    }
    else if (output == MTX_CMM_OUTPUT_ANSI)
    {
        /* unlike vt100, ANSI CSI reset is all-or-nothing */
        self->priv->tags.em_start = "\033[3m";
        self->priv->tags.em_end = "\033[0m";
        self->priv->tags.strong_start = "\033[1m";
        self->priv->tags.strong_end = "\033[0m";
        self->priv->tags.code_span_start = "\033[1;94m";
        self->priv->tags.code_span_end = "\033[0m";
        self->priv->tags.codeblock_start = "\033[1;94m";
        self->priv->tags.codeblock_end = "\033[0m\n";
        self->priv->tags.strikethrough_start = "--";
        self->priv->tags.strikethrough_end = "--";
        self->priv->tags.h1_start = "\033[1;91m";
        self->priv->tags.h1_end = "\033[0m\n";
        self->priv->tags.h2_start = "\033[1;36m";
        self->priv->tags.h2_end = "\033[0m\n";
        self->priv->tags.h3_start = "\033[1;36m";
        self->priv->tags.h3_end = "\033[0m\n";
        self->priv->tags.h4_start = "\033[1;36m";
        self->priv->tags.h4_end = "\033[0m\n";
        self->priv->tags.h5_start = "\033[1;36m";
        self->priv->tags.h5_end = "\033[0m\n";
        self->priv->tags.h6_start = "\033[1;36m";
        self->priv->tags.h6_end = "\033[0m\n";
        self->priv->tags.blockquote_start = "\033[1;36m" ">";
        self->priv->tags.blockquote_end = "\033[0m\n";
        self->priv->tags.olist_start = "";
        self->priv->tags.olist_end = "";
        self->priv->tags.ulist_start = "";
        self->priv->tags.ulist_end = "";
        self->priv->tags.li_start[0] =   /* render_open_li_block */
            "\033[93m*" "\033[0m";
        self->priv->tags.li_start[1] =   /* render_open_li_block */
            "\033[93m-" "\033[0m";
        self->priv->tags.li_end = "\n";  /* render_close_li_block */
        self->priv->tags.rule = " " "\033[93m-----" "\033[0m\n";
        self->priv->tags.para_start = "";       /* render_open_p_block */
        self->priv->tags.para_end = "\n";       /* render_close_p_block */
        self->priv->tags.br = "\n";
        self->priv->tags.table_start = "";
        self->priv->tags.table_end = "";
        self->priv->tags.thead_start = "";
        self->priv->tags.thead_end = "";
        self->priv->tags.tbody_start = "";
        self->priv->tags.tbody_end = "";
        self->priv->tags.tr_start = "";
        self->priv->tags.tr_end = "│\n";
        self->priv->tags.th_start = "│ ";
        self->priv->tags.th_end = " ";
        self->priv->tags.td_start = "│ ";
        self->priv->tags.td_end = " ";
        self->priv->tags.link_builder = mtx_cmm_linkbuilder_ansi;
        self->priv->tags.image_builder = mtx_cmm_imagebuilder_ansi;

        /* MTX plain text */
    }
    else if (output == MTX_CMM_OUTPUT_TEXT)
    {
        self->priv->tags.em_start = "";
        self->priv->tags.em_end = "";
        self->priv->tags.strong_start = "";
        self->priv->tags.strong_end = "";
        self->priv->tags.code_span_start = "";
        self->priv->tags.code_span_end = "";
        self->priv->tags.codeblock_start = "";
        self->priv->tags.codeblock_end = "\n";
        self->priv->tags.strikethrough_start = "";
        self->priv->tags.strikethrough_end = "";
        self->priv->tags.h1_start = "[";
        self->priv->tags.h1_end = "]\n";
        self->priv->tags.h2_start = "-";
        self->priv->tags.h2_end = "-\n";
        self->priv->tags.h3_start = "~";
        self->priv->tags.h3_end = "~\n";
        self->priv->tags.h4_start = "|";
        self->priv->tags.h4_end = "|\n";
        self->priv->tags.h5_start = "{";
        self->priv->tags.h5_end = "}\n";
        self->priv->tags.h6_start = "<";
        self->priv->tags.h6_end = ">\n";
        self->priv->tags.blockquote_start = ">";
        self->priv->tags.blockquote_end = "";
        self->priv->tags.olist_start = "";
        self->priv->tags.olist_end = "";
        self->priv->tags.ulist_start = "";
        self->priv->tags.ulist_end = "";
        self->priv->tags.li_start[0] = "*";     /* render_open_li_block  */
        self->priv->tags.li_start[1] = "-";     /* render_open_li_block  */
        self->priv->tags.li_end = "\n";         /* render_close_li_block */
        self->priv->tags.rule = " -----\n";
        self->priv->tags.para_start = "";       /* render_open_p_block */
        self->priv->tags.para_end = "\n";       /* render_close_p_block */
        self->priv->tags.br = "\n";
        self->priv->tags.table_start = "";
        self->priv->tags.table_end = "";
        self->priv->tags.thead_start = "";
        self->priv->tags.thead_end = "";
        self->priv->tags.tbody_start = "";
        self->priv->tags.tbody_end = "";
        self->priv->tags.tr_start = "";
        self->priv->tags.tr_end = "│\n";
        self->priv->tags.th_start = "│ ";
        self->priv->tags.th_end = " ";
        self->priv->tags.td_start = "│ ";
        self->priv->tags.td_end = " ";
        self->priv->tags.link_builder = mtx_cmm_linkbuilder_text;
        self->priv->tags.image_builder = mtx_cmm_imagebuilder_text;
    }

    /* unknown */
    else
    {
        g_warning ("unknown output enum");
        ret = FALSE;
    }

    /* save if valid */
    if (ret)
        self->priv->output = output;
    return ret;
} /* *INDENT*ON* */

#ifdef MTX_DEBUG
static void mtx_dump_queue(gpointer instance, int fd, GQueue* queue, gboolean print_junk);
#endif

/**
mtx_cmm_format_link:
Combined stash_link_dest and link_builder for the current output format.
@self:  the MtxCmm instance.
@text:  nullable.
@dest:  nullable.
@title: nullable.
Return: formatted link (string). The caller owns the memory.
*/
static inline gchar *
mtx_cmm_format_link (MtxCmm *self,
                     const gchar *text,
                     const gchar *dest,
                     const gchar *title)
{
    guint id = -1;
    if (dest)
    {
        id = mtx_cmm_stash_link_dest (self, dest);
    }
    return self->priv->tags.link_builder (self, text, dest, title, id);
}

/**
mtx_cmm_format_image:
Combined stash_link_dest and image_builder for the current output format.
@self:  the MtxCmm instance.
@text:  nullable.
@dest:  nullable.
@title: nullable.
Return: formatted image (string). The caller owns the memory.
*/
static inline gchar *
mtx_cmm_format_image (MtxCmm *self,
                      const gchar *text,
                      const gchar *dest,
                      const gchar *title)
{
    guint id = -1;
    if (dest)
    {
        id = mtx_cmm_stash_link_dest (self, dest);
    }
    return self->priv->tags.image_builder (self, text, dest, title, id);
}

/**
mtx_cmm_regex_directives:
Return: GRegex* matcher for %%directives.
*/
static GRegex *
mtx_cmm_regex_directives (MtxCmm *self)
{
    static GRegex **regex = NULL;
/*
/
(\R|^)%%(?|(nopot)[ \t]+(.*?)|(textdomain)[ \t]+(.*?))($|\R)
/gm
*/
    if (regex == NULL)
    {
        regex = &g_array_index (self->priv->regex_table, GRegex *,
                                MTX_CMM_REGEX_DIRECTIVE);
    }
    if (*regex == NULL)
    {
        GError *err = NULL;
        *regex = g_regex_new (
            "(\\R|^)%%(?|"                    /* group  1 in callback */
              "(nopot)[ \\t]+(.*?)"           /* groups 2 and 3 in callback */
              "|(textdomain)[ \\t]+(.*?)"     /* groups 2 and 3 in callback */
            ")($|\\R)"                        /* group  4 in callback */
            "", 0, 0, &err);
        if (err != NULL)
        {
            g_printerr ("directive regex: %s\n", err->message);
            g_error_free (err);
        }
    }
    return *regex;
}

/**
mtx_replace_directive_cb:
Erase mdview3 legacy directives.
*/
static gboolean
mtx_replace_directive_cb (const GMatchInfo *info,
                          GString *res,
                          gpointer data __attribute__((unused)))
{
    g_autofree gchar *b = g_match_info_fetch (info, 1);
    g_autofree gchar *e = g_match_info_fetch (info, 4);
    g_string_append (res, *b ? b : e);
    return FALSE;
}

/**
mtx_cmm_string_replace_directives:
Replace directives with nothing (erase).
@self: MtxCmm instance.
@str: GString
*/
static void
mtx_cmm_string_replace_directives (MtxCmm *self,
                                   GString *str)
{
    GRegex *regex = mtx_cmm_regex_directives (self);
    GError *err = NULL;
    g_autofree gchar *temp =
    g_regex_replace_eval (regex, str->str, -1, 0, 0,
                          mtx_replace_directive_cb, self, &err);
    if (err != NULL)
    {
        g_printerr ("%s internal error:\t%s\n", __FUNCTION__, err->message);
        g_error_free (err);
        return;
    }
    g_string_assign (str, temp);
}

/**
mtx_cmm_regex_word_split:
Return: GRegex* matcher to split segment on word separators.
*/
static GRegex *
mtx_cmm_regex_word_split (MtxCmm *self)
{
    static GRegex **regex = NULL;
/*
(?<!\\)([\p{Zs}\v\x{F600}\x{F60A}\x{F60B}\x{F608}\x{F609}\x{F60F}\x{F601}]+)
*/

    if (regex == NULL)
    {
        regex = &g_array_index (self->priv->regex_table, GRegex *,
                                MTX_CMM_REGEX_WORD_SPLIT);
    }
    if (*regex == NULL)
    {
        GError *err = NULL;
        *regex = g_regex_new (
            "(?<!\\\\)(["
              "\\p{Zs}\\v"
              rUNIPUA_BR
              rUNIPUA_B1
              rUNIPUA_B0
              rUNIPUA_E1
              rUNIPUA_E0
              rUNIPUA_QUOT
              rUNIPUA_CODE
            "]+)"
            "", 0, 0, &err);
        if (err != NULL)
        {
            g_printerr ("word split regex: %s\n", err->message);
            g_error_free (err);
        }
    }
    return *regex;
}

/**
mtx_cmm_discover_auto_code_spans:
Discover and protect auto code spans.
@text: markdown text to be searched for auto code spans.
@prefix: insert prefix before the span prior to protecting.
@prefix: append suffix after the span prior to protecting.
Return: newly-allocated markdown text with code_refs to discovered auto code
spans.  NULL if no auto code spans were discovered.
*/
static gchar *
mtx_cmm_discover_auto_code_spans (MtxCmm *self,
                                  const gchar *text,
                                  const gchar *prefix,
                                  const gchar *suffix)
{
    gchar *p;
    GError *err = NULL;
    GRegex *regex = mtx_cmm_regex_word_split (self);
    gchar **words;

    words = g_regex_split_full (regex, text, strlen (text), 0, 0, 0, &err);
    if (err != NULL)
    {
        g_printerr ("internal error: %s\n", err->message);
        g_error_free (err);
    }

/* Generous approximation, see mtx_cmm_regex_code_ref. */
#define CODE_REF_SIZE     (16 + 2 * sizeof sUNIPUA_CODE)

    for (gint j = 0; words[j] != NULL; j++)
    {
        gint start = -1;
        gchar *word = words[j];
        gint cwl = strlen (word);

        if (cwl <= 3)
        {
            continue;
        }
        /*
        Allow backlash to escape the potential code word.
        Escaping # requires two backslashes because MD4C strips one level.
        */
        if (*word == '\\')
        {
            /* Excused from potential code words. */
            memmove (words[j], words[j] + 1, cwl - 1);
            words[j][cwl - 1] = '\0';
            continue;
        }
        if (*word && mtx_word_type (word, &start, &cwl) !=
            MTX_CMM_WORD_UNKNOWN && cwl > 0)
        {
            /* Span's *s(tart) and *e(nd) characters within word. */
            gchar *s = word + start;
            gchar *e = s + cwl - 1;
            gchar *w = word;
            gint i, ref_start;
            guint len = strlen (words[j]) + strlen (prefix) + strlen (suffix);
            gchar *buf = g_malloc0 ((len + CODE_REF_SIZE + 1) * sizeof *word);

            /* Copy text before the span. */
            for (i = 0; w != s; w++)
            {
                buf[i++] = *w;
            }
            ref_start = i;

            /* Insert prefix before the span. */
            for (p = (gchar *) prefix; *p; p++)
            {
                buf[i++] = *p;
            }
            /*
            Copy span while deleting unescaped interior backslashes
            (presumably used to escape white space word splitting).
            */
            for (gboolean b = FALSE; w != e; w++)
            {
                switch (*w)
                {
                case '\\':
                    b = !b;
                    if (b)
                    {
                        continue;
                    }
                    /* fall through */
                default:
                    buf[i++] = *w;
                    b = FALSE;
                }
            }
            /* End span. */
            buf[i++] = *w++;

            /* Append suffix after the span. */
            for (p = (gchar *) suffix; *p; p++)
            {
                buf[i++] = *p;
            }
            /* Replace code_ref for discovered span. */
            buf[i + 1] = '\0';
            if ((p = mtx_cmm_protect (self, buf + ref_start)))
            {
                len = strlen (p);
                memcpy (buf + ref_start, p, len);
                g_free (p);

                /* Copy text after the span. */
                for (i = ref_start + len; *w; w++)
                {
                    buf[i++] = *w;
                }
                buf[i] = '\0';

                /* Add result to the word list. */
                g_free (words[j]);
                words[j] = buf;

              /*---------*/
                continue;
              /*---------*/
            }

            /*
            This line reached when:
            - Auto code span not discovered: fall back on input word; OR
            - mtx_cmm_protect error: give up on this span.
            */
        }
    }
    p = g_strjoinv ("", words);
    g_strfreev (words);
    return p;
}

/**
mtx_cmm_replace_auto_code_spans:
Modify string by discovering, formatting and replacing auto code spans with
code_refs.
@self: MtxCmm instance.
@target: string to modify.
@start: replace from this position to the end position included.
@end: end position.
@prefix: insert prefix before code span.
@suffix: append suffix before code span.
*/
static void
mtx_cmm_replace_auto_code_spans (MtxCmm *self,
                                 GString *target,
                                 const guint start,
                                 const guint end,
                                 const gchar *prefix,
                                 const gchar *suffix)
{
    gchar *spans, save;
    /*
    Discover auto code spans.  Discovery returns protected spans.
    */
    save = target->str[end];
    target->str[end] = '\0';
    spans = mtx_cmm_discover_auto_code_spans (self, target->str + start,
                                              prefix, suffix);
    target->str[end] = save;
    if (spans == NULL)
    {
      return;
    }
    /* Combine spans into target string */
    g_string_erase (target, start, end - start);
    g_string_insert (target, start, spans);
    g_free (spans);
}

/**
mtx_cmm_regex_dumb_quote_pairs:
Return: GRegex* matcher for dumb quote pairs.
What matches by example:
match: 'a' (at line start/end or inside line/block)
match: "a" (at line start/end or inside line/block)
no match: it's' (first quote flanked by letters)
no match: a"b" (ditto)
match: 'it's'"a" (first double quote preceded by punctuation.
no match: '' "" (both empty)
*/
static GRegex *
mtx_cmm_regex_dumb_quote_pairs (MtxCmm *self)
{
    static GRegex **regex = NULL;
/*
(?<B>^|[\p{Zs}\p{P}\x{F600}])(?<L>['"\x{F60F}])(?<M>.+?)(?<R>\g{L})(?=$|[\p{Zs}\p{P}\x{F600}])
*/

    if (regex == NULL)
    {
        regex = &g_array_index (self->priv->regex_table, GRegex *,
                                MTX_CMM_REGEX_DUMB_QUOTE_PAIR);
    }
    if (*regex == NULL)
    {
        GError *err = NULL;
        *regex = g_regex_new (
            "(?<B>^|[\\p{Zs}\\p{P}"             /* group 1 in callback */
                rUNIPUA_BR
            "])"
              "(?<L>['\""                       /* group 2 in callback */
                rUNIPUA_QUOT
              "])"
                  "(?<M>.+?)"                   /* group 3 in callback */
              "(?<R>\\g{L})"
            "(?=$|[\\p{Zs}\\p{P}"
                rUNIPUA_BR
            "])"
              "", 0, 0, &err);
        if (err != NULL)
        {
            g_printerr ("dumb quote pair regex: %s\n", err->message);
            g_error_free (err);
        }
    }
  return *regex;
}

/**
mtx_replace_dumb_quote_pair_cb:
*/
static gboolean
mtx_replace_dumb_quote_pair_cb (const GMatchInfo *info,
                                GString *res,
                                gpointer data __attribute__((unused)))
{
    gchar *B, *L, *M;
    gchar *l, *r;

    B = g_match_info_fetch (info, 1);
    L = g_match_info_fetch (info, 2);
    M = g_match_info_fetch (info, 3);
    if (*L == '\'')
    {
        l = "‘", r = "’";
    }
    else
    {
        l = "“", r = "”";
    }
    g_string_append_printf (res, "%s%s%s%s", B, l, M, r);
    g_free (M);
    g_free (L);
    g_free (B);

    return FALSE;
}

/**
mtx_cmm_string_replace_smart_quotes:
Replace dumb quote pairs with smart quotes.
@self: MtxCmm instance.
@str: GString
*/
static void
mtx_cmm_string_replace_smart_quotes (MtxCmm *self,
                                     GString *str)
{
    GRegex *regex = mtx_cmm_regex_dumb_quote_pairs (self);
    GError *err = NULL;

    gchar *temp =
    g_regex_replace_eval (regex, str->str, -1, 0, 0,
                          mtx_replace_dumb_quote_pair_cb, self, &err);
    if (err)
    {
      g_printerr ("%s internal error:\t%s\n", __FUNCTION__, err->message);
      g_error_free (err);
      return;
    }
    g_string_assign (str, temp);
    g_free (temp);
}

/**
mtx_cmm_replace_smart_text:
Modify @target string making text "smart".
@self: MtxCmm instance.
@target: string to modify.
@start: replace from this position to the end position included.
@end: end position.
*/
static void
mtx_cmm_replace_smart_text (MtxCmm *self,
                            GString *target,
                            const guint start,
                            const guint end)
{
    GString *work;
    gchar *p;
    enum
    {
        APOS  = 1 << 0,
        QUOT  = 1 << 1,
        MDASH = 1 << 2,
    } found = 0x0;

    /* Analyze target for ALL potential matches. */
    for (p = target->str + start; *p && p <= target->str + end; p++)
    {
        switch (*p)
        {
        case '\'':
            /* Ignore, it's, let's and so on... */
            if ((ptrdiff_t) (p - target->str) - start == 0
                || !isalpha (*(p - 1)) || !isalpha (*(p + 1)))
            {
                found |= APOS;
            }
            break;
        case cUNIPUA_QUOT2:
            if ((ptrdiff_t) (p - target->str) - start > 1
                && *(p - 1) == cUNIPUA_QUOT1 && *(p - 2) == cUNIPUA_QUOT0)
            {
                found |= QUOT;
            }
            break;
        case '"':
            if (!self->priv->escaping)
            {
                found |= QUOT;
            }
            break;
        case '&':
            if (self->priv->escaping && *(p + 1) == 'q')
            {
                found |= QUOT;
            }
            break;
        case '-':
            if (*(p + 1) == '-')
            {
                found |= MDASH;
            }
            break;
        }
    }
    if (!found)
    {
        return;
    }
    /* Prepare space-padded work string. */
    work = g_string_new_len (" ", target->len - start + 2);
    memcpy (work->str + 1, target->str + start, target->len - start);
    *(work->str + (work->len - 1)) = ' ';

    if (found & MDASH)
    {
        g_string_replace (work, " -- ", " — ", 0);
    }
    if (found & APOS)
    {
        mtx_cmm_string_replace_smart_quotes (self, work);
    }
    if (found & QUOT)
    {
        mtx_cmm_string_replace_smart_quotes (self, work);
    }
    g_string_truncate (target, start);
    /* adjusted for padding */
    g_string_append_len (target, work->str + 1, work->len - 2);
    g_string_free (work, TRUE);
}

/**
mtx_cmm_regex_unipua:
Return: GRegex* matcher to match UNIPUA singletons.
The render_* functions insert singletons in lieu of some output tags.
*/
static GRegex *
mtx_cmm_regex_unipua (MtxCmm *self)
{
    static GRegex **regex = NULL;

    if (regex == NULL)
    {
        regex = &g_array_index (self->priv->regex_table, GRegex *,
                                MTX_CMM_REGEX_UNIPUA);
    }
    if (*regex == NULL)
    {
        GError *err = NULL;
        *regex = g_regex_new (
            rUNIPUA_BR   "|"
            rUNIPUA_B1   "|"
            rUNIPUA_B0   "|"
            rUNIPUA_E1   "|"
            rUNIPUA_E0   "|"
            rUNIPUA_AMP  "|"
            rUNIPUA_LT   "|"
            rUNIPUA_GT   "|"
            rUNIPUA_QUOT
            "", 0, 0, &err);
        if (err != NULL)
        {
            g_printerr ("unipua regex: %s\n", err->message);
            g_error_free (err);
        }
    }
    return *regex;
}

/**
mtx_replace_unipua_cb:
*/
static gboolean
mtx_replace_unipua_cb (const GMatchInfo *info,
                       GString *res,
                       gpointer data)
{
    gchar * match = g_match_info_fetch (info, 0);
    gchar *r = g_hash_table_lookup ((GHashTable *)data, match);
    g_string_append (res, r);
    g_free (match);
    return FALSE;
}

/**
mtx_cmm_string_release_unipua:
Replace all UNIPUA singletons in @str with the formatted text they stand for.
@str: GString
*/
static void
mtx_cmm_string_release_unipua (MtxCmm *self,
                               GString *str)
{
    GRegex *regex = mtx_cmm_regex_unipua (self);
    GError *err = NULL;

    GHashTable *h = g_hash_table_new (g_str_hash, g_str_equal);

    GString *tag_br = g_string_new (self->priv->tags.br);
    if (self->priv->output == MTX_CMM_OUTPUT_HTML)
    {
        g_string_append (tag_br, self->priv->tweaks & MTX_CMM_TWEAK_HTML5 ?
                         ">\n" : " />\n");
    }
    g_hash_table_insert (h, sUNIPUA_BR, tag_br->str);
    g_hash_table_insert (h, sUNIPUA_E1, (gchar *) self->priv->tags.em_start);
    g_hash_table_insert (h, sUNIPUA_E0, (gchar *) self->priv->tags.em_end);
    g_hash_table_insert (h, sUNIPUA_B1,
                         (gchar *) self->priv->tags.strong_start);
    g_hash_table_insert (h, sUNIPUA_B0, (gchar *) self->priv->tags.strong_end);

    /* render_html_escaped */
    if (self->priv->escaping)
    {
        g_hash_table_insert (h, sUNIPUA_AMP, "&amp;");
        g_hash_table_insert (h, sUNIPUA_LT, "&lt;");
        g_hash_table_insert (h, sUNIPUA_GT, "&gt;");
        g_hash_table_insert (h, sUNIPUA_QUOT, "&quot;");
    }
    else
    {
        g_hash_table_insert (h, sUNIPUA_AMP, "&");
        g_hash_table_insert (h, sUNIPUA_LT, "<");
        g_hash_table_insert (h, sUNIPUA_GT, ">");
        g_hash_table_insert (h, sUNIPUA_QUOT, "'");
    }
    g_autofree gchar *temp =
    g_regex_replace_eval (regex, str->str, -1, 0, 0,
                          mtx_replace_unipua_cb, h, &err);
    g_string_free (tag_br, TRUE);
    g_hash_table_destroy (h);
    if (err)
    {
        g_printerr ("%s internal error:\t%s\n", __FUNCTION__, err->message);
        g_error_free (err);
        return;
    }
    g_string_assign (str, temp);
}

/**
mtx_cmm_regex_tilde_code_fence:

Return: GRegex* matcher for markdown markdown `~` code fence.
*/
static const GRegex *
mtx_cmm_regex_tilde_code_fence (MtxCmm *self)
{
    static GRegex **regex = NULL;

    if (regex == NULL)
    {
        regex = &g_array_index (self->priv->regex_table, GRegex *,
                                MTX_CMM_REGEX_TILDE_CODE_FENCE);
    }
    if (*regex == NULL)
    {
        GError *err = NULL;
        *regex = g_regex_new ("(^|\\R) {0,3}~{3,}+", 0, 0, &err);
        if (err != NULL)
        {
            g_printerr ("tilde_code_fence regex: %s\n", err->message);
            g_error_free (err);
        }
    }
    return *regex;
}

/**
mtx_cmm_str_tilde_code_fence_max_len:
Return: length of the longest markdown `~` code fence in the input string.
*/
static guint
mtx_cmm_str_tilde_code_fence_max_len (MtxCmm *self, const gchar *str)
{
    GMatchInfo *match_info;
    const GRegex *regex = mtx_cmm_regex_tilde_code_fence (self);
    guint ret = 0; /* also in case of errors */
    if (g_regex_match_all (regex, str, 0, &match_info))
    {
        ret = strlen (g_match_info_fetch (match_info, 0));
    }
    g_match_info_free (match_info);
    return ret;
}

/**
mtx_cmm_parser_get_unit_head:
Get the head (top) unit.
*/
__attribute__((unused))
static inline MtxCmmParserUnit *mtx_cmm_parser_get_unit_head (MtxCmm *);

static inline MtxCmmParserUnit *
mtx_cmm_parser_get_unit_head (MtxCmm *self)
{
    /* stack: x -- x */
    g_assert(g_queue_get_length (self->priv->unitq) > 0);
    return (MtxCmmParserUnit *) self->priv->unitq_head;
}

/**
mtx_cmm_parser_find_unit_index:
Return index of the first queue unit that intersects type and flag.
@type: %MtxCmmParserUnitType bit mask.
@flag: %MtxCmmParserUnitFlag bit mask.
@start: start searching a match from queue index @start.
@unitptr: pointer to the matching unit. Nullable.
Return: -1 if no unit matches otherwise return the index of the
matching unit and a set *@unit.  Head has index zero.
The instance owns the returned memory. You should not free it.
*/
/*static*/ int
mtx_cmm_parser_find_unit_index (MtxCmm *self,
                                const MtxCmmParserUnitType type,
                                const MtxCmmParserUnitFlag flag,
                                const int start,
                                MtxCmmParserUnit **unitptr)
{
    MtxCmmParserUnit *unit;
    for (gint i = start; (unit = g_queue_peek_nth (self->priv->unitq, i)); i++)
    {
        if (unit->type & type && unit->flag & flag)
        {
            if (unitptr)
            {
                *unitptr = unit;
            }
            return i;
        }
    }
    return -1;
}

/**
mtx_cmm_parser_unit_ends_with_c:
*/
static inline gboolean
mtx_cmm_parser_unit_ends_with_c (MtxCmmParserUnit *unit,
                                 gchar c)
{
    gchar *p;
    return
        unit->args ? (unit->args->len > 0
                      && (p = g_array_index (unit->args, gchar *,
                                             unit->args->len - 1)) && p ?
                      p[strlen (p) - 1] == c : FALSE)
        : (unit->text && unit->text->len > 0 ?
           unit->text->str[unit->text->len - 1] == c : FALSE);
}

/**
mtx_cmm_parser_top_unit_ends_line:
*/
/*static inline*/ gboolean
mtx_cmm_parser_top_unit_ends_line (MtxCmm *self)
{
    return mtx_cmm_parser_unit_ends_with_c ((MtxCmmParserUnit *)
                                            self->priv->unitq_head, '\n');
}

/**
mtx_cmm_parser_get_unit_arg_under:
Return argument of first queue unit from top that intersects type and flag.
@type: %MtxCmmParserUnitType bit mask.
@flag: %MtxCmmParserUnitFlag bit mask.
@index: argument index.
Return: NULL if no unit matches or the argument doesn't exist.
The instance owns the returned memory. You should not free it.
*/
__attribute__((unused))
static const gchar
*mtx_cmm_parser_get_unit_arg_under (MtxCmm *,
                                   const MtxCmmParserUnitType,
                                   const MtxCmmParserUnitFlag,
                                   const guint);

static const gchar *
mtx_cmm_parser_get_unit_arg_under (MtxCmm *self,
                                   const MtxCmmParserUnitType type,
                                   const MtxCmmParserUnitFlag flag,
                                   const guint index)
{
    MtxCmmParserUnit *unit;
    for (gint i = 0; (unit = g_queue_peek_nth (self->priv->unitq, i)); i++)
    {
        if (unit->type & type && unit->flag & flag)
        {
            if (unit->args && unit->args->len > index)
            {
                return g_array_index (unit->args, gchar *, index);
            }
            break;
        }
    }
    return NULL;
}

/**
mtx_cmm_parser_merge_down_unit_arg:
Coalesce opening ARG unit's text to the next argument of the unit below.
*/
/*
Detail: pop closing ARG unit then append a copy of the opening ARG unit's text
to the argument list of the unit below it.  If opening ARG's text is NULL then a
NULL argument will be appended.  Pop two queue units (ARG ARG) and move them to
the junk queue.
*/
static void
mtx_cmm_parser_merge_down_unit_arg (MtxCmm *self)
{
    MtxCmmParserUnit *arg, *below;
    gchar *value;

    /* stack: ARG ARG receiver -- receiver */
    g_assert (g_queue_get_length (self->priv->unitq) > 2);

    g_assert (((MtxCmmParserUnit *) self->priv->unitq_head)->type ==
              MTX_CMM_PARSER_UNIT_ARG);
    mtx_cmm_parser_unit_pop_head (self);            /* ARG ARG -- ARG */
    g_assert (((MtxCmmParserUnit *) self->priv->unitq_head)->type ==
              MTX_CMM_PARSER_UNIT_ARG);
    arg = mtx_cmm_parser_unit_pop_head (self);      /* ARG --      */
    below =
    (MtxCmmParserUnit *) self->priv->unitq_head;    /* receiver   */

    g_assert (below->args != NULL);
    value = arg->text ? g_strdup (arg->text->str) : NULL;
    g_array_append_val (below->args, value);
}

/**
mtx_cmm_parser_merge_down_unit_arg_inlines:
Coalesce inlines between the opening and closing ARG_INLINES into the next
argument of the unit below the opening ARG_INLINES.
*/
/*
Detail: Reach the opening ARG_INLINES unit then collect inlines until the
closing ARG_INLINES then append the collected text to the argument list of the
unit below the opening ARG_INLINES.  If all contained inlines have NULL text
then a NULL argument will be appended. Image units (SPAN_IMG) get special
treatment[1].  Pop multiple queue units (ARG_INLINES... ARG_INLINES) and move
them to the junk queue.

[1] SPAN_A and SPAM_IMG can contain spans. A can contain CODE and IMG.  IMG
can contain CODE, A and even IMG.  CODE isn't a problem because it only has
a ->text field. A and IMG only have ->args fields, which normally - when not
nested - are formatted by mtx_cmm_parser _after_ the call to mtx_cmm_render.
Thus here, in mtx_cmm_render, we need to format the spans ourselves. Note we
only need do IMG formatting because A is contained but can't contain IMG.
And since a image nesting is unrolled before this stage, we only need to
format simple IMG units holding their usual three ->args.
*/
static void mtx_cmm_render_link_unit (MtxCmm *, MtxCmmParserUnit *, gchar *());
static void
mtx_cmm_parser_merge_down_unit_arg_inlines (MtxCmm *self)
{
    MtxCmmParserUnit *p, *below;
    gchar *value;
    gint i, j;
    GString *buf = NULL;

    /* stack: ARG_INLINES... ARG_INLINES receiver -- receiver */
    g_assert (g_queue_get_length (self->priv->unitq) > 2);

    g_assert (((MtxCmmParserUnit *) self->priv->unitq_head)->type ==
              MTX_CMM_PARSER_UNIT_ARG_INLINES);
    p = mtx_cmm_parser_unit_pop_head (self);    /* ARG_INLINES... -- ... */

    /* Reach the opening ARG_INLINES. */
    for (i = 0;
         (p = (MtxCmmParserUnit *) g_queue_peek_nth (self->priv->unitq, i))
         && p->type != MTX_CMM_PARSER_UNIT_ARG_INLINES; i++)
        ;
    /* Collect inlines above until top unit. */
    for (j = i;
         j >= 0 && (p = (MtxCmmParserUnit *) g_queue_peek_nth
                    (self->priv->unitq, j)); j--)
    {
        if (p->type == MTX_CMM_PARSER_UNIT_SPAN_IMG)
        {
            gchar *ref;
            mtx_cmm_render_link_unit (self, p, mtx_cmm_format_image);
            if ((ref = mtx_cmm_protect (self, p->text->str)))
            {
                g_string_assign (p->text, ref);
                g_free (ref);
            }
        }
        if (p->text)
        {
            if (buf)
            {
                g_string_append (buf, p->text->str);
            }
            else
            {
                buf = g_string_new (p->text->str);
            }
        }
    }
    do
    {
        /* ... ARG_INLINES receiver -- receiver */
        p = mtx_cmm_parser_unit_pop_head (self);
    }
    while (--i >= 0);

    below = (MtxCmmParserUnit *) self->priv->unitq_head;  /* receiver   */
    g_assert (below->args != NULL);
    if (buf)
    {
        value = buf->str;
        g_string_free (buf, FALSE);
    }
    else
    {
        value = NULL;
    }
    g_array_append_val (below->args, value);
}

/**
mtx_cmm_render_link_unit:
*/
static void
mtx_cmm_render_link_unit (MtxCmm *self,
                          MtxCmmParserUnit *unit,
                          MtxCmmAImgFormatter *formatter)
{
    gchar *temp, *ref;
    enum
    {
        DEST, TITLE, TEXT
    };
    gchar *dest = g_array_index (unit->args, gchar *, DEST);
    gchar *title = g_array_index (unit->args, gchar *, TITLE);
    gchar *text = g_array_index (unit->args, gchar *, TEXT);

    /* text[0] indicates whether the link unit is in a <table> context. */
    self->priv->inside_table = text[0] == '1'; /* for formatter() */

    /* text[1] == 0 means that markdown link text is empty */
    if (text[1] && (self->priv->extensions & MTX_CMM_EXTENSION_SMART_TEXT))
    {
        GString *str = g_string_new (text + 1);
        mtx_cmm_replace_smart_text (self, str, 0, str->len);
        text = str->str;
        g_string_free (str, FALSE);
        g_array_remove_index (unit->args, TEXT);
        g_array_insert_val (unit->args, TEXT, text);
    }
    else
    {
        text++;
    }

    /* FIXME: auto-code should be applied to text */
    temp = formatter (self, text[0] ? text : NULL, dest, title);
    if ((ref = mtx_cmm_protect (self, temp)))
    {
        if (unit->text)
        {
            g_string_assign (unit->text, ref);
        }
        else
        {
            unit->text = g_string_new (ref);
        }
        g_free (ref);
    }
    self->priv->inside_table = FALSE;
    g_free (temp);
}

/*********************************************************************
*                          STAGE 1 RENDERER                          *
*********************************************************************/

#include "mtxrender.h"

static inline int
mtx_cmm_render (MtxCmm *self,
                const MD_CHAR * input,
                MD_SIZE input_size,
                void (*process_output) (const MD_CHAR *, MD_SIZE, void *),
                void *userdata __attribute__((unused)),
                unsigned parser_flags,
                unsigned renderer_flags)
{
    return md_mtx (input, input_size, process_output, self, parser_flags,
                   renderer_flags);
}


/*********************************************************************
*                       PARSER & RENDERER CODA                       *
*********************************************************************/

/**
mtx_cmm_render_process_output:
mtx_render callback.
*/
inline static void
mtx_cmm_render_process_output (const MD_CHAR *out,
                               MD_SIZE length,
                               void *userdata)
{
    MtxCmm *self = userdata;
    MtxCmmParserUnit *head = (MtxCmmParserUnit *) self->priv->unitq_head;

    self->priv->seen_unit_types |= head->type;

    switch (head->type)
    {
        /* Use of MTX_CMM_PARSER_UNIT_JUNK is reserved for mtx_cmm_mtx. */
        /* Inconsequential programming error if found here. */
    case MTX_CMM_PARSER_UNIT_JUNK:
        break;

        /* NULL stack bottom noop corresponding to MD4C's start of document. */
    case MTX_CMM_PARSER_UNIT_NULL:
        break;

        /* Append argument to current unit's args array. */
    case MTX_CMM_PARSER_UNIT_ARG:
        if (head->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
        {
            mtx_cmm_parser_merge_down_unit_arg (self);
            break;
        }
        /* fall through */

        /* Append inlines to the nearest opening ARG_INLINES. */
    case MTX_CMM_PARSER_UNIT_ARG_INLINES:
        if (head->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
        {
            mtx_cmm_parser_merge_down_unit_arg_inlines (self);
            break;
        }
        /* fall through */

    default:
        /* Grow current unit's text field. */
        if (length > 0 || head->type == MTX_CMM_PARSER_UNIT_ARG)
        {
            if (head->text == NULL)
                head->text = g_string_new_len (out, length);
            else
                g_string_append_len (head->text, out, length);
        }
        break;
    }
}

#ifdef MTX_DEBUG
/**********************************************************************
*                           DEBUGGING TIPS                            *
**********************************************************************/
/*
Function mtx_dump_queue is useful to inspect the stack.  In gdb invoke
`p mtx_dump_queue (X,2,0,1)` where X can be "self" or "r->userdata" or
"userdata" depending on the location of the current frame.
*/

/**
mtx_dump_queue:
Debug: dump parser GQueue queue.
@instance: parser instance whose `initq` is dumped if @queue is NULL.
@fd: output file descriptor
@queue: parser queue to dump, nullable.
@print_junk:
*/
__attribute__((unused))
static void mtx_dump_queue (gpointer, int, GQueue *, gboolean);

static void
mtx_dump_queue (gpointer instance,
                int fd,
                GQueue *queue,
                gboolean print_junk)
{
    MtxCmmParserUnit *unit;
    gchar *temp;

    if (queue == NULL)
    {
        queue = ((MtxCmm *) instance)->priv->unitq;
    }
    dprintf (fd, "%5s%c%3s%5s %4s %9s %s   [STACK BOTTOM]\n", "UNIT", ',',
             "ARG", "TYPE", "FLAG", "ADDR", "TEXT AND ARGS");
    for (gint i = g_queue_get_length (queue) - 1; i >= 0; i--)
    {
        gchar type_str[16];
        gchar flag_str[16] = { ' ' };

        unit = (MtxCmmParserUnit *) g_queue_peek_nth (queue, i);
        if (unit->type == MTX_CMM_PARSER_UNIT_JUNK && !print_junk)
        {
            continue;
        }
        switch (unit->type)
        {
        case MTX_CMM_PARSER_UNIT_ARG:
            strcpy (type_str, "arg");
            break;
        case MTX_CMM_PARSER_UNIT_ARG_INLINES:
            strcpy (type_str, "argin");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_CODE:
            strcpy (type_str, "bcode");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_H:
            strcpy (type_str, "h");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_HTML:
            strcpy (type_str, "html");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_HR:
            strcpy (type_str, "hr");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_LI:
            strcpy (type_str, "li");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_OL:
            strcpy (type_str, "ol");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_P:
            strcpy (type_str, "p");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_QUOTE:
            strcpy (type_str, "bquot");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_UL:
            strcpy (type_str, "ul");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_TABLE:
            strcpy (type_str, "table");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_THEAD:
            strcpy (type_str, "thead");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_TBODY:
            strcpy (type_str, "tbody");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_TR:
            strcpy (type_str, "tr");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_TH:
            strcpy (type_str, "th");
            break;
        case MTX_CMM_PARSER_UNIT_BLOCK_TD:
            strcpy (type_str, "td");
            break;
        case MTX_CMM_PARSER_UNIT_INLINES:
            strcpy (type_str, "inlns");
            break;
        case MTX_CMM_PARSER_UNIT_JUNK:
            strcpy (type_str, "junk");
            break;
        case MTX_CMM_PARSER_UNIT_NULL:
            strcpy (type_str, "null");
            break;
        case MTX_CMM_PARSER_UNIT_SPAN_A:
            strcpy (type_str, "link");
            break;
        case MTX_CMM_PARSER_UNIT_SPAN_CODE:
            strcpy (type_str, "scode");
            break;
        case MTX_CMM_PARSER_UNIT_SPAN_IMG:
            strcpy (type_str, "img");
            break;
        default:
            snprintf (type_str, 16, "%5x", unit->type);
        }
        temp = flag_str;
        *temp++ = unit->flag & MTX_CMM_PARSER_UNIT_FLAG_ARGS ? 'a' : ' ';
        *temp++ = unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN ? 'o' : ' ';
        *temp++ = unit->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE ? 'c' : ' ';
        if (unit->text)
        {
            temp = g_strescape (unit->text->str, NULL);
            dprintf (fd, "% 5d%c%2s %5s %4s %p \"%s\"\n", i, ' ', "", type_str,
                     flag_str, unit->text->str, temp);
            g_free (temp);
        }
        if (unit->args)
        {
            if (unit->args->len == 0)
            {
                dprintf (fd, "% 5d%c%2s %5s %4s %p %s\n", i, ' ', "", type_str,
                         flag_str, unit->args, "[no args]");
            }
            else
            {
                for (guint j = 0; j < unit->args->len; j++)
                {
                    gchar *arg =
                        (gchar *) g_array_index (unit->args, gchar *, j);
                    if (arg)
                    {
                        temp = g_strescape (arg, NULL);
                        dprintf (fd, "% 5d%c%-2d %5s %4s %p \"%s\"\n", i, ',',
                                 j, type_str, flag_str, arg, temp);
                        g_free (temp);
                    }
                    else
                    {
                        dprintf (fd, "% 5d%c%-2d %5s %4s %p %s\n", i, ',', j,
                                 type_str, flag_str,
                                 ((gchar *) (unit->args->data)) + j,
                                 "[null arg]");
                    }
                }
            }
        }
        if (!unit->text && !unit->args)
        {
            dprintf (fd, "% 5d%c%2s %5s %4s %p %s\n", i, ' ', "", type_str,
                     flag_str, unit, "" /*"[empty unit]" */ );
        }
    }
}
#endif

/**
_col_strlen:
Return string length measured in units of a terminal column.  This function
takes our internal use of Unicode PUA codepoints into consideration.
*/
static gint
_col_strlen (const gchar *p)
{
    glong len = 0;
    g_return_val_if_fail (p != NULL, -1);

    while (*p)
    {
        gunichar ch = g_utf8_get_char (p);
        if (!
            (g_unichar_iszerowidth (ch)
             || (iUNIPUA_E1 <= ch && ch <= iUNIPUA_B0)))
        {
            len += g_unichar_iswide (ch) ? 2 : 1;
        }
        p = g_utf8_next_char (p);
    }
    return len;
}

/**
mtx_cmm_mtx:
Convert markdown to the desired output format.

@self:
@markdown: address of a pointer to the markdown string.
@size: pointer to the size of the returned string. NULLABLE.
@clear_markdown: if TRUE, free *@markdown and set @markdown to NULL as early as
possible during the conversion process.

- Set the desired X with `mtx_cmm_set_output` before calling this function.
- Enable @clear_markdown to curb peak heap allocation.

`mtx_cmm_mtx` expects valid UTF-8 input text and does not validate.  It is
recommended to validate input with `g_utf8_validate` before calling
`mtx_cmm_mtx` otherwise results could be unpredictable.

Return: a newly-allocated string holding X, and set *@size to the size of
the returned string in bytes. On error, it returns NULL and *@size is
undefined.  In both cases if clear_markdown is TRUE, *@markdown is freed
and *@markdown is set to NULL.
*/
/*
Conversion takes place in two stages. The first stage (mtx_render)
closely interfaces with MD4C. The interface is derived from the MD4C
html renderer (md4c-html.c). In the first stage, render_* functions
push text units on the MtxCmmParserUnit ->priv->unitq stack. In the
second stage (mtx_cmm_mtx render CODA), units are popped to apply
transformations, and joined together for final output.
*/
gchar *
mtx_cmm_mtx (MtxCmm *self,
             gchar **markdown,
             gsize *size,
             const gboolean clear_markdown)
{
    g_return_val_if_fail (MTX_IS_CMM (self), FALSE);
    g_return_val_if_fail (self->priv->output != MTX_CMM_OUTPUT_UNKNOWN, FALSE);

    mtx_cmm_mtx_reset (self);

    if (!(*markdown && *markdown[0]))
    {
        if (clear_markdown)
        {
            g_free (*markdown);
            *markdown = NULL;
        }
        if (size != NULL)
        {
            *size = 0;
        }
        return g_strdup ("");
    }
#if MTX_DEBUG > 2
    gchar *phase;
#endif
    GString *ret;
    gint i;
    gchar *ref, *temp;
    MtxCmmParserUnit *unit;
    GQueue *unitq = self->priv->unitq;
    const gboolean do_autocode =
        self->priv->extensions & MTX_CMM_EXTENSION_AUTO_CODE;
    const gboolean do_permlink =
        self->priv->extensions & MTX_CMM_EXTENSION_PERMLINK;
    gboolean in_shebang = FALSE;
    const gboolean do_shebang =
        self->priv->extensions & MTX_CMM_EXTENSION_SHEBANG;
    const gboolean do_smart_text =
        self->priv->extensions & MTX_CMM_EXTENSION_SMART_TEXT;
    gboolean do_tables =
        self->priv->extensions & MTX_CMM_EXTENSION_TABLE;
    gboolean do_margin = self->priv->output == MTX_CMM_OUTPUT_PANGO;
    self->priv->escaping = self->priv->escape
        || self->priv->output == MTX_CMM_OUTPUT_HTML;
    ret = g_string_new (*markdown);
    if (clear_markdown)
    {
        g_free (*markdown);
        *markdown = NULL;
    }

    /*********************************************************************
    *                         SHEBANG EXTENSION                          *
    *********************************************************************/
    if (do_shebang)
    {
        gchar *p = ret->str;
        if (*p++ == '#' && *p++ == '!')
        {
            for (; *p == ' ' || *p == '\t'; p++)
                ;
            if (*p && *p++ == '/')
            {
                in_shebang = TRUE;
                gint m = mtx_cmm_str_tilde_code_fence_max_len (self, ret->str);
                g_autofree gchar *fence = g_strnfill (m > 0 ? m + 1 : 3, '~');
                g_autofree gchar *line = g_strdup_printf ("%s\n", fence);
                g_string_prepend (ret, line);
            }
        }
    }

    /*********************************************************************
    *                         ERASE %%DIRECTIVES                         *
    *********************************************************************/

    /*
    Since MTX doesn't support gettext extraction and replacement,
    erase legacy directives for compatibility with existing documents.
    */
    if (!in_shebang)
    {
        mtx_cmm_string_replace_directives (self, ret);
    }

#if MTX_DEBUG > 1
    g_printerr ("@@@@@@@@@@ markdown:\n%s\n@@@@@@@@@@\n", ret->str);
#endif

#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                           PARSE MARKDOWN                           *\n\
    **********************************************************************";
#endif

    self->priv->seen_unit_types = 0;
    i =
    mtx_cmm_render (self, ret->str, ret->len, mtx_cmm_render_process_output,
                    NULL,
                    (do_tables ? MD_FLAG_TABLES : 0) |
                    MD_FLAG_STRIKETHROUGH |
                    (do_permlink ? MD_FLAG_PERMISSIVEAUTOLINKS : 0),
                    MD_HTML_FLAG_SKIP_UTF8_BOM | MD_HTML_FLAG_XHTML);
    g_string_free (ret, TRUE);
    if (i < 0)
    {
        return NULL;
    }
#if MTX_DEBUG > 2
    g_printerr ("%s\n", phase);
    mtx_dump_queue (self, 2, self->priv->unitq, TRUE);
#endif

    /*******************************************************************
    *                         RENDERING CODA                           *
    *******************************************************************/

    if (do_tables &&
        !(self->priv->seen_unit_types & MTX_CMM_PARSER_UNIT_BLOCK_TABLE))
    {
        do_tables = FALSE; /* nothing to do */
    }



#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                  CONSOLIDATE TEXT AND ARGUMENTS                    *\n\
    **********************************************************************";
#endif

    /*
    The following loop consolidates unit->text and ->args. This process results
    in simpler units, where ->args are no longer needed[1], and pre-output[2]
    text is chunked across ->text fields of queue units.

    [1] Block unit ->args are not consolidated here to allow further processing.
    [2] Pre-output chunks still need text transforms applied, such as auto-codes
    and smart text. In some cases, e.g. link, image and code span, a pre-output
    chunk is encoded with a code_ref to prevent tampering by text transforms.
    */
    /*
    The loop starts with a section that tidies up raw HTML inlines
    and HTML blocks.  Essentially it involves shortening text unless
    self->priv->unsafe_html.  Note that raw HTML is only seen here if HTML
    output mode is active.
    */
    /*.
    Then the loop visits all units on queue that have ->args to modify
    unit->text based on ->args and unit->type. For instance, before the visit,
    a link span will have no text and three arguments (link destination, link
    text and link title). After the visit, its ->text will consist of the
    fully-rendered and protected link.  Consolidated units with ->args are moved
    out of the way to the trash queue.  Freeing ->args (or ->text) here is never
    necessary as the queues are automatically cleaned up before exit.
    */
    /*
    The loop ends with a bunch of pass-through case labels to prevent
    programming errors.  To add more MtxCmmParserUnitType values consider
    adding them to the pass-through set unless handled by an earlier case label.
    */

    for (i = g_queue_get_length (unitq) - 1; i >= 0; i--)
    {
        unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);
        switch (unit->type)
        {
        /*********************************************************************
        *                               RAW HTML                             *
        *********************************************************************/

        /*
        Raw HTML from render_text_html can stand for an inline tag or a line
        belonging to a larger HTML block.  If unsafe_html is on render the tag
        or the block as is otherwise render SAFE_HTML for each tag or whole
        block. Protect the outcome.
        */
        case MTX_CMM_PARSER_UNIT_BLOCK_HTML:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
            {
                continue;
            }
            /* fall through */
        case MTX_CMM_PARSER_UNIT_RAW_HTML:     /* inline tag */
            ref = mtx_cmm_protect (self, (self->priv->tweaks &
                                          MTX_CMM_TWEAK_UNSAFE_HTML) ?
                                   unit->text->str : SAFE_HTML);
            if (ref)
            {
                g_string_assign (unit->text, ref);
                g_free (ref);
            }
            if (unit->type == MTX_CMM_PARSER_UNIT_BLOCK_HTML
                && !(self->priv->tweaks & MTX_CMM_TWEAK_UNSAFE_HTML))
            {
                g_string_append_c (unit->text, '\n');
            }
            break;




        /*********************************************************************
        *                        UNITS WITH ARGUMENTS                        *
        *********************************************************************/


        /*********************************************************************
        *                            INLINE SPANS                            *
        *********************************************************************/

        /* mtx_cmm_render_link_unit renders and protects its output. */

        case MTX_CMM_PARSER_UNIT_SPAN_A:
            mtx_cmm_render_link_unit (self, unit,
                                      (MtxCmmAImgFormatter *)
                                      mtx_cmm_format_link);
            break;

        case MTX_CMM_PARSER_UNIT_SPAN_IMG:
            mtx_cmm_render_link_unit (self, unit,
                                      (MtxCmmAImgFormatter *)
                                      mtx_cmm_format_image);
            break;

        case MTX_CMM_PARSER_UNIT_SPAN_CODE:
            if ((ref = mtx_cmm_protect (self, unit->text->str)))
            {
                g_string_assign (unit->text, ref);
                g_free (ref);
            }
            break;

        /*********************************************************************
        *                                BLOCKS                              *
        *********************************************************************/

        case MTX_CMM_PARSER_UNIT_BLOCK_CODE:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                if (!(self->priv->tweaks & MTX_CMM_TWEAK_CM_BLOCK_END)
                    && unit->text && unit->text->len
                    && unit->text->str[unit->text->len - 1] == '\n')
                {
                    /* Apply option --cm-block-end. */
                    g_string_truncate (unit->text, unit->text->len - 1);
                }
            }
            else
            {
                MtxCmmParserUnit *below;

                g_assert (unit->text == NULL);
                g_assert (unit->args->len == 1);
                below = g_queue_peek_nth (self->priv->unitq, i + 1);
                g_assert (below && below->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN);
                if (self->priv->output == MTX_CMM_OUTPUT_TEXT && below->text
                    == NULL)
                {
                    below->text = g_string_new ("");
                }
                else
                {
                    g_assert (below->text != NULL);
                }
                g_string_append (below->text, g_array_index (unit->args, gchar
                                                             *, 0));
                mtx_cmm_parser_unit_consume (&unit);
                if ((ref = mtx_cmm_protect (self, below->text->str)))
                {
                    g_string_assign (below->text, ref);
                    g_free (ref);
                }
            }
            break;


        case MTX_CMM_PARSER_UNIT_BLOCK_P:
        case MTX_CMM_PARSER_UNIT_BLOCK_LI:
        case MTX_CMM_PARSER_UNIT_BLOCK_H:
        case MTX_CMM_PARSER_UNIT_BLOCK_TABLE:
        case MTX_CMM_PARSER_UNIT_BLOCK_THEAD:
        case MTX_CMM_PARSER_UNIT_BLOCK_TBODY:
        case MTX_CMM_PARSER_UNIT_BLOCK_TR:
        case MTX_CMM_PARSER_UNIT_BLOCK_TH:
        case MTX_CMM_PARSER_UNIT_BLOCK_TD:
        /* Blocks that have args holding their start and end tags. */
        /* Postpone claiming args. */

            /*noop */ break;

        /*********************************************************************
        *                             PASS-THROUGH                           *
        *********************************************************************/

        case MTX_CMM_PARSER_UNIT_NULL:
        case MTX_CMM_PARSER_UNIT_BLOCK_QUOTE:
        case MTX_CMM_PARSER_UNIT_BLOCK_HR:
        case MTX_CMM_PARSER_UNIT_BLOCK_OL:
        case MTX_CMM_PARSER_UNIT_BLOCK_UL:
        case MTX_CMM_PARSER_UNIT_ARG:
        case MTX_CMM_PARSER_UNIT_ARG_INLINES:
        case MTX_CMM_PARSER_UNIT_INLINES:
        case MTX_CMM_PARSER_UNIT_JUNK:

            /*noop */ break;

        /*********************************************************************
        *                          PROGRAMMING ERROR                         *
        *********************************************************************/

        default:
            g_assert (*"unhandled case label" == '!');
        }
    }

#if MTX_DEBUG > 2
    g_printerr ("%s\n", phase);
    mtx_dump_queue (self, 2, self->priv->unitq, TRUE);
#endif





#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                            COLLAPSE BLOCKS                         *\n\
    **********************************************************************";
#endif

    /*
    Inline units inside block units are collapsed into the text field of their
    opening block unit.  Stack-wise this means that between an opening block
    unit and its corresponding closing unit there is a mix of inline units, such
    as link, image and code SPANs, and text, HTML entities and raw HTML tags
    inside CONTAINERs. The ->text fields of these inlines are concatenated and
    the result is appended to the opening block unit's ->text.
    */

    /*
    P and LI interplay because a LI pair can also contain P pairs in addition to
    the inline mix mentioned above. This happens for loose lists. In this case
    LI delegates text harvesting to the contained P blocks.
    */
    for (i = g_queue_get_length (unitq) - 1; i >= 0; i--)
    {
        unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);
        MtxCmmParserUnit *curr;

        switch (unit->type)
        {
        case MTX_CMM_PARSER_UNIT_JUNK:
            continue;


        case MTX_CMM_PARSER_UNIT_BLOCK_H:
        case MTX_CMM_PARSER_UNIT_BLOCK_P:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                /* arg[0] = tags.<block>_start */
                g_assert (unit->args && unit->args->len == 1);
                g_assert (unit->text == NULL);

                /* Harvest text fields up to my closing unit. */
                while ((curr = (MtxCmmParserUnit *) g_queue_peek_nth
                        (unitq, --i))
                       && !(curr->type & unit->type
                            && curr->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE))
                {
                    if (curr->type != MTX_CMM_PARSER_UNIT_JUNK && curr->text
                        && curr->text->len)
                    {
                        if (unit->text)
                        {
                            g_string_append (unit->text, curr->text->str);
                        }
                        else
                        {
                            unit->text = g_string_new (curr->text->str);
                        }
                    }
                    mtx_cmm_parser_unit_consume (&curr);
                }

                /********************
                *  At closing unit  *
                ********************/

                if (curr && self->priv->output == MTX_CMM_OUTPUT_HTML)
                {
                    g_assert (curr->text == NULL);
                    g_assert (curr->args && curr->args->len == 1);
                }

                /* Markdown doesn't produce empty paragraphs. */
                if (unit->type == MTX_CMM_PARSER_UNIT_BLOCK_P && curr &&
                    unit->text == NULL)
                {
                    mtx_cmm_parser_unit_consume (&unit);
                    mtx_cmm_parser_unit_consume (&curr);
                }
            }
            break;


        case MTX_CMM_PARSER_UNIT_BLOCK_LI:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                /* arg[0] = tags.li_start */
                /* arg[1] = is_tight 'T'|'F'                */
                /* arg[2] = level                           */
                /* arg[3] = ordinal - negative for <ul>     */
                g_assert (unit->args && unit->args->len == 4);
                g_assert (unit->text == NULL);

                /* In loose lists the contained P harvests text. */
                gboolean is_tight =
                *((gchar *) g_array_index (unit->args, gchar *, 1)) == 'T';

                if (is_tight)
                {
                    /* Harvest text fields up to my closing unit  */
                    /* or to the start of a sub-list.             */
                    unit->text = g_string_new ("");
                    while ((curr = (MtxCmmParserUnit *) g_queue_peek_nth
                            (unitq, --i))
                           && !((curr->type & MTX_CMM_PARSER_UNIT_BLOCK_LI
                                 && curr->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
                                || (curr->type & (MTX_CMM_PARSER_UNIT_BLOCK_OL |
                                                  MTX_CMM_PARSER_UNIT_BLOCK_UL)
                                    && curr->
                                    flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)))
                    {
                        if (curr->type != MTX_CMM_PARSER_UNIT_JUNK &&
                            curr->text && curr->text->len)
                        {
                            if (unit->text)
                            {
                                g_string_append (unit->text, curr->text->str);
                            }
                            else
                            {
                                unit->text = g_string_new (curr->text->str);
                            }
                        }
                        if (curr->type != MTX_CMM_PARSER_UNIT_BLOCK_QUOTE)
                        {
                            mtx_cmm_parser_unit_consume (&curr);
                            /*
                            List item and block quote units stand for containers
                            blocks, which don't consume each other.
                            */
                        }
                    }

                    /********************
                    *  At closing unit  *
                    ********************/

                    if (curr && curr->type == MTX_CMM_PARSER_UNIT_BLOCK_LI)
                    {
                        /* arg[0] = tags.li_end                     */
                        g_assert (curr->text == NULL);
                        g_assert (curr->args && curr->args->len == 1);
                    }
                }
            }
            break;


        case MTX_CMM_PARSER_UNIT_BLOCK_TH:
        case MTX_CMM_PARSER_UNIT_BLOCK_TD:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                if (self->priv->output != MTX_CMM_OUTPUT_HTML)
                {
                    /* arg[0] = tags.li_start */
                    g_assert (unit->args && unit->args->len == 1);
                }
                /* Harvest text fields up to my closing unit. */
                while ((curr = (MtxCmmParserUnit *) g_queue_peek_nth
                        (unitq, --i))
                       && !(curr->type & unit->type
                            && curr->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE))
                {
                    if (curr->type != MTX_CMM_PARSER_UNIT_JUNK && curr->text
                        && curr->text->len)
                    {
                        if (unit->text)
                        {
                            g_string_append (unit->text, curr->text->str);
                        }
                        else
                        {
                            unit->text = g_string_new (curr->text->str);
                        }
                    }
                    mtx_cmm_parser_unit_consume (&curr);
                }
            }
            break;


        default: ;     /* hush -Wswitch warning */
        }
    }

#if MTX_DEBUG > 2
    g_printerr ("%s\n", phase);
    mtx_dump_queue (self, 2, self->priv->unitq, FALSE);
#endif






#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                 ELIDE BLOCK QUOTE ELEMENTS (PANGO)                 *\n\
    **********************************************************************";
#endif

    /*
    Here, for Pango only, we elide certain block quote units.  Specifically, we
    can change a closing and an immediately following opening units to junk
    without affecting the look[1] of Pango output.

    [1] Eliminating such units affects the structure of the document but we
    will insert enough block quote properties into Pango markup that the
    Pango application will still be able to format block quotes correctly.
    */

    if (do_margin
        && (self->priv->seen_unit_types & MTX_CMM_PARSER_UNIT_BLOCK_QUOTE))
    {
        MtxCmmParserUnit *above;
        for (i = g_queue_get_length (unitq) - 1; i > 0; i--)
        {
            unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);

            if (unit->type == MTX_CMM_PARSER_UNIT_BLOCK_QUOTE
                && unit->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
            {
                for (gint j = i - 1; j >= 0; j--)
                {
                    above = g_queue_peek_nth (self->priv->unitq, j);
                    if (above->type != MTX_CMM_PARSER_UNIT_JUNK)
                    {
                        break;
                    }
                }
                if (above->type == MTX_CMM_PARSER_UNIT_BLOCK_QUOTE &&
                    above->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
                {
                    mtx_cmm_parser_unit_consume (&unit);
                    mtx_cmm_parser_unit_consume (&above);
                    i += 2;            /* backtrack to assess container unit */
                }
            }
        }
#if MTX_DEBUG > 2
        g_printerr ("%s\n", phase);
        mtx_dump_queue (self, 2, self->priv->unitq, TRUE);
#endif
    }






#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *               PREPROCESS MARKDOWN TABLES (NOT HTML)                *\n\
    **********************************************************************";
#endif

    /*
    Here we preprocess tables for all output modes but HTML, taking the
    _col_length of each cell, then storing the maximum length of each column,
    and saving a serialized array as arg[0] of the <table> unit.  The lengths
    will be used further down to justify the formatted columns.
    Note: we do assume monospace font for _col_length to make sense at all.
    */

    if (do_tables && self->priv->output != MTX_CMM_OUTPUT_HTML)
    {
        GPtrArray *max_col_width = NULL;
        guint curr_col = -1;
        glong cmax, clen;

        /* Read reversed tables, from </table> to <table> */
        for (i = 0; i < (gint) g_queue_get_length (unitq) - 1; i++)
        {
            unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);

            switch (unit->type)
            {
            case MTX_CMM_PARSER_UNIT_BLOCK_TR:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
                {
                    curr_col = -1;
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TD:
            case MTX_CMM_PARSER_UNIT_BLOCK_TH:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
                {
                    GString *str;
                    gchar *prefix;

                    g_assert (unit->text->len); /*{N,L,C,R}...*/
                    ++curr_col;
                    if (curr_col >= max_col_width->len)
                    {
                        g_ptr_array_add (max_col_width, 0);
                    }
                    cmax = (glong) g_ptr_array_index (max_col_width, curr_col);

                    /* clen is the length in terminal columns (monospace font)
                       of the text that will be eventually rendered */
                    str = g_string_new (unit->text->str);
                    mtx_dbg_errout (-1, "(%s) strlen=%lu", unit->text->str,
                                   unit->text->len);
                    (void)mtx_cmm_string_release_protected_unmarked (self, str);
                    mtx_dbg_errseq (-1, " => (%s) strlen=%lu", str->str,
                                    str->len);
                    clen = _col_strlen (str->str);
                    /* pass clen downstream to the section that will
                       justify cell contents */
                    prefix = g_strdup_printf ("%ld:", clen);
                    g_string_insert (unit->text, 0, prefix);
                    g_free (prefix);
                    g_string_free (str, TRUE);
                    mtx_dbg_errseq (-1, " => (%s)\n", unit->text->str);

                    if (clen > cmax)
                    {
                        gpointer *pptr =
                        &g_ptr_array_index (max_col_width, curr_col);
                        *pptr = (gpointer) (glong) clen;
                    }
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TABLE:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
                {
                    max_col_width = g_ptr_array_new ();
                }
                else
                {
                    GString *serialize = (GString *) g_string_new ("");
                    for (gint c = max_col_width->len - 1; c >= 0; c--)
                    {
                        g_string_append_printf (serialize, "%ld ", (glong)
                                                g_ptr_array_index
                                                (max_col_width, c));
                    }
                    gchar **a0 = &g_array_index (unit->args, gchar *, 0);
                    g_free (*a0);
                    *a0 = serialize->str;
                    g_string_free (serialize, FALSE);
                    g_ptr_array_free (max_col_width, TRUE);
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TBODY:
            case MTX_CMM_PARSER_UNIT_BLOCK_THEAD:
                break;

            default: ;     /* hush -Wswitch warning */
            }
        }
#if MTX_DEBUG > 2
        g_printerr ("%s\n", phase);
        mtx_dump_queue (self, 2, self->priv->unitq, TRUE);
#endif
    }





#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                 JUSTIFY MARKDOWN TABLES (NOT HTML)                 *\n\
    **********************************************************************";
#endif

    /*
    Here we justify table contents for all output modes but HTML.  Arg[0] of
    the <table> unit holds: <max_width_col1> <max_width_col2>...  Each <th>
    and <td> unit starts with its alignment code {'N','L','C','R'} followed by
    the cell text proper.
    */

    if (do_tables && self->priv->output != MTX_CMM_OUTPUT_HTML)
    {
        GPtrArray *max_col_width = NULL;
        gint curr_col = -1;
        gint cmax, clen, len, end, bufend;
        gchar *a0, *buf = NULL, *p;

        for (i = g_queue_get_length (unitq) - 1; i > 0; i--)
        {
            unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);

            switch (unit->type)
            {
            case MTX_CMM_PARSER_UNIT_BLOCK_TABLE:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
                {
                    max_col_width = g_ptr_array_new ();
                    a0 = g_array_index (unit->args, gchar *, 0);
                    gchar *pos = NULL;
                    gint tmax = 0;
                    gchar *token = strtok_r (a0, " ", &pos);

                    while (token != NULL)
                    {
                        if ((cmax = atoi (token)) > tmax)
                        {
                            tmax = cmax;
                        }
                        g_ptr_array_add (max_col_width,
                                         (gpointer) (glong) cmax);
                        token = strtok_r (NULL, " ", &pos);
                    }
                    bufend = 4 * tmax; /* UTF-8 */
                    buf = g_malloc (bufend + 2);
                }
                else
                {
                    g_ptr_array_free (max_col_width, TRUE);
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TR:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
                {
                    curr_col = -1;
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TD:
            case MTX_CMM_PARSER_UNIT_BLOCK_TH:
                if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
                {
                    g_assert (unit->text->len); /*"."*/
                    ++curr_col;
                    if (unit->text == NULL)
                    {
                        break;
                    }
                    cmax = (glong) g_ptr_array_index (max_col_width, curr_col);
                    clen = atoi (unit->text->str);
                    gchar *src = strchr (unit->text->str, ':') + 1;
                    /* corrected for atoi':' */
                    len = unit->text->len - (src - unit->text->str);
                    /* corrected for the {'N','L','C','R'} before src */
                    len -= 1; /* bytes */

                    /* justify text */
                    switch (src[0])
                    {
                    case 'N':
                    case 'L':
                        p = buf + len;
                        memcpy (p - len, src + 1, len);
                        memset (p, ' ', end = cmax - clen);
                        p[end] = '\0';
                        break;
                    case 'R':
                        p = buf + (cmax - clen);
                        memset (buf, ' ', (cmax - clen));
                        memcpy (p, src + 1, end = len);
                        p[end] = '\0';
                        break;
                    case 'C':
                        p = buf + (cmax - clen) / 2;
                        memset (buf, ' ', (cmax - clen) / 2);
                        memcpy (p, src + 1, len);
                        memset (p + len, ' ', end = (cmax - clen + 1) / 2);
                        p += len;
                        p[end] = '\0';
                        break;
                    default:
                        ;
                    }

                    g_string_assign (unit->text, g_array_index (unit->args,
                                                                gchar *, 0));
                    g_string_append (unit->text, buf);
                }
                break;
            case MTX_CMM_PARSER_UNIT_BLOCK_TBODY:
            case MTX_CMM_PARSER_UNIT_BLOCK_THEAD:
                break;

            default: ;     /* hush -Wswitch warning */
            }
        }
        g_free (buf);
#if MTX_DEBUG > 2
        g_printerr ("%s\n", phase);
        mtx_dump_queue (self, 2, self->priv->unitq, TRUE);
#endif
    }





#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                        TRANSFORM BLOCK TEXT                        *\n\
    *                  (AND CLAIM REMAINING ARGUMENTS)                   *\n\
    *               INSERT INDENTATION PROPERTIES (PANGO)                *\n\
    **********************************************************************";
#endif

    /*
    By now inlines are protected so as not to entangle text transformations,
    including smart text and auto-code discovery.
    */
    /*
    Here we also add Pango <span> properties to assist applications that will
    indent blockquote and list blocks.
    */

    guint blockquote_level = 0, ol_ul_level = 0;
    gchar *copy_of_blockquote_open_str = NULL;
    for (i = g_queue_get_length (unitq) - 1; i >= 0; i--)
    {
        unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);

        switch (unit->type)
        {
        case MTX_CMM_PARSER_UNIT_JUNK:
            continue;

        case MTX_CMM_PARSER_UNIT_BLOCK_H:
        case MTX_CMM_PARSER_UNIT_BLOCK_P:

            g_assert (unit->args && unit->args->len == 1);
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                /* arg[0] = tags.<block>_start */
                g_assert (unit->args && unit->args->len == 1);
                if (unit->text)
                {
                    if (do_autocode)
                    {
                        mtx_cmm_replace_auto_code_spans
                            (self, unit->text, 0,
                             unit->text->len,
                             self->priv->tags.code_span_start,
                             self->priv->tags.code_span_end);
                    }
                    if (do_smart_text)
                    {
                        mtx_cmm_replace_smart_text (self, unit->text, 0,
                                                    unit->text->len);
                    }
                }

                /* Insert start tag. */
                temp = (gchar *) g_array_index (unit->args, gchar *, 0);
                if (unit->text)
                {
                    g_string_prepend (unit->text, temp);
                }
                else
                {
                    unit->text = g_string_new (temp);
                }
            }
            else     /* Closing unit. */
            {
                /* arg[0] = tags.<block>_end */
                g_assert (unit->args && unit->args->len == 1);
                /* Insert end tag. */
                unit->text =
                    g_string_new ((gchar *) g_array_index (unit->args, gchar
                                                           *, 0));
            }
            break;


        case MTX_CMM_PARSER_UNIT_BLOCK_LI:
            if (unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN)
            {
                /* arg[0] = tags.li_start */
                /* arg[1] = is_tight 'T'|'F'                */
                /* arg[2] = level                           */
                /* arg[3] = ordinal - negative for <ul>     */
                g_assert (unit->args && unit->args->len == 4);

                gboolean is_tight =
                *((gchar *) g_array_index (unit->args, gchar *, 1)) == 'T';

                if (unit->text)
                {
                    if (is_tight)      /* If not the contained BLOCK_P will do it. */
                    {
                        if (do_autocode)
                        {
                            mtx_cmm_replace_auto_code_spans
                                (self, unit->text,
                                 0, unit->text->len,
                                 self->priv->tags.code_span_start,
                                 self->priv->tags.code_span_end);
                        }
                        if (do_smart_text)
                        {
                            mtx_cmm_replace_smart_text (self, unit->text, 0,
                                                        unit->text->len);
                        }
                    }
                }
                else
                {
                    unit->text = g_string_new ("");
                }

                /* Insert start tag. */
                if (self->priv->output != MTX_CMM_OUTPUT_PANGO)
                {
                    temp = (gchar *) g_array_index (unit->args, gchar *, 0);
                    g_string_prepend (unit->text, temp);
                }
                else             /* Pango */
                {
                    /* For Pango also include list item properties. */
                    temp = (gchar *) g_array_index (unit->args, gchar *, 0);
                    /*
                    NEVER embed LI contents in a font="..." span due to the
                    limitation of our TextView's _text_buffer_insert_markup()
                    */
                    temp =
                        g_strdup_printf
                        ("<span font=\"@%s%s%s%s%s%ld%s%d\">%s</span>",
                         _tag_info[MTX_TAG_LI_LEVEL],
                         (gchar *) g_array_index (unit->args, gchar *, 2),
                         _tag_info[MTX_TAG_LI_ORDINAL],
                         (gchar *) g_array_index (unit->args, gchar *, 3),
                         _tag_info[MTX_TAG_LI_BULLET_LEN],
                         g_utf8_strlen (temp, -1), _tag_info[MTX_TAG_LI_ID],
                         i, temp);
                    g_string_prepend (unit->text, temp);
                    g_free (temp);
                    g_string_append_printf (unit->text,
                                            "<span font=\"@%s%d\">%s</span>",
                                            _tag_info[MTX_TAG_LI_ID], i,
                                            sUNIPUA_PANGO_EMPTY_SPAN);
                }
            }
            else                /* Closing unit. */
            {
                /* arg[0] = tags.li_end                     */
                g_assert (unit->args && unit->args->len == 1);

                /* Insert end tag. */
                /*
                Note: arg0 is - and should remain - NULL when
                render_close_li_block collapses a run of closing LI tags.
                */
                unit->text =
                g_string_new ((gchar *) g_array_index (unit->args, gchar *, 0));
            }
            break;


        /*********************************************************************
        *                             BLOCK QUOTE                            *
        *********************************************************************/
        /*
        Here we adjust block quote rendering for Pango applications.  Output
        ->priv->tag->blockquote_start/end as usual and also prepend/append a
        _tag_info element conveying blockquote level.
        Blockquote start/end tags can be collapsed to minimize vertical white
        space. In order to do so, we control newlines here, and require newline
        not to be included in the start/end tags for Pango output mode.
        */

        case MTX_CMM_PARSER_UNIT_BLOCK_QUOTE:
            if (do_margin)
            {
                MtxCmmParserUnit *above = NULL;
                gchar gap[128];
                gboolean collapse, open;

                open = unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN;
                for (gint j = i - 1; j >= 0; j--)
                {
                    above = g_queue_peek_nth (self->priv->unitq, j);
                    if (above->type != MTX_CMM_PARSER_UNIT_JUNK)
                        break;
                }

                /* Initialize indentation gap string.                       */
                /* Insert filler to keep Pango from dropping an empty span. */
                if (unit->text == NULL || unit->text->len == 0)
                {
                    unit->text =
                    g_string_new (open ? "    " : sUNIPUA_PANGO_EMPTY_SPAN);
                }
                collapse =
                    above ? (above->type == MTX_CMM_PARSER_UNIT_BLOCK_QUOTE
                             && above->
                             flag & (open ? MTX_CMM_PARSER_UNIT_FLAG_OPEN :
                                     MTX_CMM_PARSER_UNIT_FLAG_CLOSE)) : FALSE;
                /* Markdown /^>+ +$/ sets block quote level = number of ">" */

                /*
                On open unit set indentation before blockquote text.
                On close unit set indentation after blockquote text.
                blockquote_level is 0 outside MTX_CMM_PARSER_UNIT_BLOCK_QUOTE.
                Value 0 is never sent to Pango.
                */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
                snprintf (gap, sizeof (gap),
                          collapse ? "" : "<span font=\"@%s%d%s%d\">",
                          _tag_info[MTX_TAG_BLOCKQUOTE_LEVEL],
                          open ? ++blockquote_level : blockquote_level--,
                          _tag_info[MTX_TAG_BLOCKQUOTE_OPEN], open != 0);
#pragma GCC diagnostic pop

                if (open)
                {
                    if (collapse)
                    {
                        g_string_assign (unit->text, "");
                    }
                    else
                    {
                        if (copy_of_blockquote_open_str == NULL)
                        {
#if MTX_TEXT_VIEW_DEBUG > 1
                            /* Replace default symbol to see who does what. */
                            GString *s = g_string_new (unit->text->str);
                            /* Don't change the byte length! */
                            g_string_replace (s, "┌", "╓", 0);
                            copy_of_blockquote_open_str = s->str;
                            g_string_free (s, FALSE);
#else
                            copy_of_blockquote_open_str =
                                g_strdup (unit->text->str);
#endif
                        }
                        g_string_prepend (unit->text, gap);
                        g_string_append (unit->text, "</span>");
                    }
                }
                else
                {
                    if (collapse)
                    {
                        g_string_assign (unit->text, "");
                    }
                    else
                    {
                        if (above && above->type ==
                            MTX_CMM_PARSER_UNIT_BLOCK_QUOTE
                            && above->flag & MTX_CMM_PARSER_UNIT_FLAG_CLOSE)
                        {
                            /* Sink run of blockquote_end
                               tags to avoid visual noise. */
                            g_string_assign (unit->text, "");
                        }
                        else
                        {
                            g_string_append (unit->text, "</span>\n");
                            g_string_prepend (unit->text, gap);
                        }

                        /*
                        Block quote level decreases but still inside block quotes:
                        inject block quote opening Pango tag to tell the Pango application
                        which is the current level.
                        */
                        if (blockquote_level)
                        {
                            snprintf (gap, sizeof (gap),
                                      "<span font=\"@%s%d%s%d\">%s</span>",
                                      _tag_info[MTX_TAG_BLOCKQUOTE_LEVEL],
                                      blockquote_level,
                                      _tag_info[MTX_TAG_BLOCKQUOTE_OPEN], TRUE,
                                      copy_of_blockquote_open_str);
                            g_string_append (unit->text, gap);
                        }
                    }
                }
            }
            break;


        /*********************************************************************
        *                             OL/UL LIST                             *
        *********************************************************************/
        /*
        Here we adjust list rendering for Pango applications.  Output
        ->priv->tag->(ol/ul)_start/end as usual and also prepend/append a
        _tag_info element conveying list level. This is all quite similar to
        blockquote except that we do not collapse endings.
        */

        case MTX_CMM_PARSER_UNIT_BLOCK_OL:
        case MTX_CMM_PARSER_UNIT_BLOCK_UL:
            if (do_margin)
            {
                gchar gap[128];
                gboolean open;

                open = unit->flag & MTX_CMM_PARSER_UNIT_FLAG_OPEN;
                if (open)
                {
                    ++ol_ul_level;
                }
                else
                {
                    --ol_ul_level;
                }
                /* Insert filler to keep Pango from dropping an empty span. */
                if (unit->text == NULL || unit->text->len == 0)
                {
                    unit->text = g_string_new (sUNIPUA_PANGO_EMPTY_SPAN);
                }
                snprintf (gap, sizeof (gap), "<span font=\"@%s%d\">",
                          _tag_info[MTX_TAG_OL_UL_LEVEL], ol_ul_level);
                if (open)
                {
                    g_string_prepend (unit->text, gap);
                    g_string_append (unit->text, "</span>");
                }
                else if (ol_ul_level == 0)   /* no need to tag higher levels */
                {
                    g_string_append (unit->text, "</span>");
                    g_string_prepend (unit->text, gap);
                }
            }
            break;

        default: ;     /* hush -Wswitch warning */
        }
    }
    g_free (copy_of_blockquote_open_str);

#if MTX_DEBUG > 2
    g_printerr ("%s\n", phase);
    mtx_dump_queue (self, 2, self->priv->unitq, FALSE);
#endif





#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                          JOIN QUEUE UNITS                          *\n\
    **********************************************************************";
#endif

    /*
    Here mtx_cmm_mtx assembles the return value by concatenating unit texts.
    Unit ->args elements, if any, that weren't merged into a unit text field
    before this point are ignored and sink for good.
    */

    ret = g_string_new ("");
    for (i = g_queue_get_length (unitq) - 1; i >= 0; i--)
    {
        unit = (MtxCmmParserUnit *) g_queue_peek_nth (unitq, i);
        if (unit->type & (MTX_CMM_PARSER_UNIT_ARG | MTX_CMM_PARSER_UNIT_JUNK))
        {
            continue;
        }
        /* Grow result value. */
        if (unit->text && unit->text->len)
        {
            g_string_append (ret, unit->text->str);
        }
    }

#if MTX_DEBUG > 2
    g_printerr ("%s\n", phase);
    mtx_dump_queue (self, 2, self->priv->unitq, FALSE);
#endif

    /* Clean up. */
    mtx_cmm_parser_clear_queues (self);

#if MTX_DEBUG > 2
    phase = "\
    **********************************************************************\n\
    *                                FIN                                 *\n\
    **********************************************************************";
    g_printerr ("%s\n", phase);
#endif

#if MTX_DEBUG > 1
    _print_priv_table (self->priv->code_table, _SOyel "%s: CODE TABLE:" _SE
                       "\n", __FUNCTION__);
    g_printerr ("@@@@@@@@@@ ret->str:\n%s\n@@@@@@@@@@\n", ret->str);
#endif

    /* Release (re)protected spans. */
    while (mtx_cmm_string_release_protected (self, ret) > 0)
        ;

    /* Release UNIPUA singletons. */
    mtx_cmm_string_release_unipua (self, ret);

    if (ret->len > 0 && ret->str[ret->len - 1] == '\n')
    {
        g_string_truncate (ret, ret->len - 1);
    }
    temp = ret->str;
    if (size != NULL)
    {
        *size = ret->len;
    }
    g_string_free (ret, FALSE);
    return temp;
}
