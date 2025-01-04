/* vim:set ts=8 sw=4 et: */
/*
MDVIEW MTX

Copyright (C) 2024-2025 step, https://github.com/step-

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

#include "mtxmarkup.h"
#include "mtxdbg.h"

struct _pod {
    GString *mkdin;
    glong len;
    MtxCmmPageMeta *meta;
};
enum {
    KEEP_TAGS,
    SKIP_TOC,
    TRACK_PAGE,
};
static gchar *reverse_xpaths[] = {
    [KEEP_TAGS]  = "keep_tags/renderer/mtx",
    [SKIP_TOC]   = "skip_toc/renderer/mtx",
    [TRACK_PAGE] = "track_page/viewer/mtx",
};
static const gchar * const elements[] = {
    "mtx", "renderer", "viewer", "keep_tags", "skip_toc", "track_page", NULL,
};
enum {
    E_UNKNOWN,
    E_MISPLACED,
};
static gchar *errfmt[] = {
    [E_UNKNOWN]   = "the '%s' element is unknown",
    [E_MISPLACED] = "the '%s' element is under the wrong parent element",
};

static GString *text = NULL;

static void
mtx_markup_elem_start (GMarkupParseContext *context,
                       const gchar *name,
                       const gchar **attr_name,
                       const gchar **attr_value,
                       gpointer user_data,
                       GError **error);
__attribute__((unused))
static void
mtx_markup_elem_end (GMarkupParseContext *context,
                     const gchar *name,
                     gpointer user_data,
                     GError **error);
__attribute__((unused))
static void
mtx_markup_text (GMarkupParseContext *context,
                 const gchar *text,
                 gsize text_len,
                 gpointer user_data,
                 GError **error);
__attribute__((unused))
static void
mtx_markup_passthrough (GMarkupParseContext *context,
                        const gchar *passthrough_text,
                        gsize text_len,
                        gpointer user_data,
                        GError **error);
static void
mtx_markup_error (GMarkupParseContext *context,
                  GError *error,
                  gpointer user_data);

/*
Parser
*/
static const GMarkupParser mtx_xml_parser = {
    mtx_markup_elem_start,
    mtx_markup_elem_end,
    mtx_markup_text,
    NULL /* mtx_markup_passthrough */,
    mtx_markup_error
};

/**
mtx_check_xpath:

@xpath: string "a/b/c..."
@stack: GSList a->b->c->...->NULL
Return: TRUE if all components/items match and zero components/items are left.
*/
static gboolean
mtx_check_xpath (const gchar *xpath,
             const GSList *stack)
{
    const gchar *q;

    for (q = strchr (xpath, '/'); q && stack->next; q = strchr(xpath, '/'))
    {
        if (strncmp (stack->data, xpath, q - xpath) != 0)
        {
            return FALSE;
        }
        xpath = q + 1;
        stack = stack->next;
    }
    return q == NULL && stack->next == NULL && strcmp (xpath, stack->data) == 0;
}

/**
mtx_markup_elem_start:
Called for opening tags like <mtx>.
*/
static void
mtx_markup_elem_start (GMarkupParseContext *context __attribute__((unused)),
                       const gchar *name,
                       const gchar **attr_name,
                       const gchar **attr_value __attribute__((unused)),
                       gpointer user_data __attribute__((unused)),
                       GError **error __attribute__((unused)))
{
    const gchar *p;

    mtx_dbg_errout (-1, "<%s>\n", name);
    for (gsize i = 0; (p = attr_name[i]); i++)
    {
        mtx_dbg_errout (-1, "attr %s = \"%s\"\n", p, attr_value[i]);
    }
    if (!g_strv_contains (elements, name))
    {
        g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                     errfmt[E_UNKNOWN], name);
    }
}

/**
mtx_markup_elem_end:
Called for closing tags like </mtx>.
*/
static void
mtx_markup_elem_end (GMarkupParseContext *context __attribute__((unused)),
                     const gchar *name,
                     gpointer user_data,
                     GError **error __attribute__((unused)))
{
    MtxCmmPageMeta *meta = ((struct _pod *) user_data)->meta;
    const GSList *stack = g_markup_parse_context_get_element_stack (context);

    mtx_dbg_errout (-1, "</%s>\n", name);
    if (strcmp (name, "keep_tags") == 0)
    {
        if (!mtx_check_xpath (reverse_xpaths[KEEP_TAGS], stack))
        {
            goto misplaced;
        }
        meta->renderer_keep_tags = atoi (text->str);
        g_string_truncate (text, 0);
    }
    if (strcmp (name, "skip_toc") == 0)
    {
        if (!mtx_check_xpath (reverse_xpaths[SKIP_TOC], stack))
        {
            goto misplaced;
        }
        meta->renderer_skip_toc = atoi (text->str);
        g_string_truncate (text, 0);
    }
    else if (strcmp (name, "track_page") == 0)
    {
        if (!mtx_check_xpath (reverse_xpaths[TRACK_PAGE], stack))
        {
            goto misplaced;
        }
        meta->viewer_track_page = atoi (text->str);
        g_string_truncate (text, 0);
    }
    return;

misplaced:
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 errfmt[E_MISPLACED], name);
}

/**
mtx_markup_text:
Called for character data. Text is not nul-terminated.
*/
static void
mtx_markup_text (GMarkupParseContext *context __attribute__((unused)),
                 const gchar *chars,
                 gsize length,
                 gpointer user_data __attribute__((unused)),
                 GError **error __attribute__((unused)))
{
    mtx_dbg_errout (-1, "[%s]\n", chars);
    g_string_append_len (text, chars, length);
}

/**
mtx_markup_passthrough:
Called for strings that should be re-saved verbatim in this same
position, but are not otherwise interpretable. At the moment this
includes comments and processing instructions. Text is not
nul-terminated.
*/
static void
mtx_markup_passthrough (GMarkupParseContext *context __attribute__((unused)),
                        const gchar *passthrough_text __attribute__((unused)),
                        gsize text_len __attribute__((unused)),
                        gpointer user_data __attribute__((unused)),
                        GError **error __attribute__((unused)))
{
    mtx_dbg_errout (-1, "passthrough: %s\n", passthrough_text);
}

/**
mtx_markup_error:
Called when any parsing method encounters
an error. The GError should not be freed.
*/
static void
mtx_markup_error (GMarkupParseContext *context __attribute__((unused)),
                  GError *error,
                  gpointer user_data)
{
    GString *mkdin = ((struct _pod *) user_data)->mkdin;
    glong len = ((struct _pod *) user_data)->len;
    g_string_truncate (mkdin, len);
    g_string_prepend (mkdin, "\n");
    g_string_prepend (mkdin, error->message);
    g_string_prepend (mkdin, "````\n");
    g_printerr ("%s\n", mkdin->str + 5);
}

/**
mtx_markup:
Parse XML prefix of markup string, and set %MtxCmmPageMeta.

@markup: marked-up string to parse.
@len: length in characters of the @markup prefix to parse.
Pass -1 to parse the entire nul-terminated @markup string.
@meta: pointer to %MtxCmmPageMeta to hold return values.

Return: TRUE for successful parsing, FALSE on parsing error. Parsing stops as
soon as an error occurs returning the values collected up to that point.
Unknown elements and attributes are ignored.
*/
/*
KNOWN ELEMENTS:
<mtx>
    <renderer>
        <!-- output all tags - allows embedding pango markup -->
        <keep_tags>1</keep_tags>
        <!-- do not add the Table of Contents regardless of --toc_level -->
        <skip_toc>1</skip_toc>
    </renderer>
    <viewer>
        <!-- do not add the page name in the navigation trail ->
        <track_page>0</track_page>
    </viewer>
</mtx>
*/
gboolean
mtx_markup_parse (GString *markup,
                  const glong len,
                  MtxCmmPageMeta *meta)
{
    struct _pod pod = { markup, len, meta };

    text = g_string_new ("");
    GMarkupParseContext *context =
    g_markup_parse_context_new (&mtx_xml_parser, G_MARKUP_PREFIX_ERROR_POSITION,
                                &pod, NULL);
    gboolean ret =
    g_markup_parse_context_parse (context, markup->str, len, NULL);
    g_markup_parse_context_free (context);
    g_string_free (text, TRUE);
    text = NULL;
    return ret;
}
