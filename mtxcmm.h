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

#ifndef MTX_CMM_H
#define MTX_CMM_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MTX_TYPE_CMM         (mtx_cmm_get_type())
G_DECLARE_FINAL_TYPE (MtxCmm, mtx_cmm, MTX, CMM, GObject)
typedef struct _MtxCmmPrivate MtxCmmPrivate;

struct _MtxCmm
{
    GObject       parent_instance;

    /*< private >*/
    MtxCmmPrivate *priv;
};

/**********************************************************************/

/**********************************************************************/


typedef enum _MtxCmmOutput
{
    MTX_CMM_OUTPUT_ANSI,
    MTX_CMM_OUTPUT_TTY,
    MTX_CMM_OUTPUT_TEXT,
    MTX_CMM_OUTPUT_PANGO,
    MTX_CMM_OUTPUT_HTML,
    MTX_CMM_OUTPUT_UNKNOWN,
} MtxCmmOutput;

typedef enum _MtxCmmExtensions
{
    MTX_CMM_EXTENSION_NONE              = 0,
    MTX_CMM_EXTENSION_SHEBANG           = 1 << 0, /* auto-codeblock at file shebang */
    MTX_CMM_EXTENSION_SMART_TEXT        = 1 << 1, /* smart quote... replacement */
    MTX_CMM_EXTENSION_AUTO_CODE         = 1 << 2, /* code span discovery */
    MTX_CMM_EXTENSION_PERMLINK          = 1 << 3, /* linkify unmarked links (MD4C) */
    MTX_CMM_EXTENSION_AUTO_LANG         = 1 << 4, /* prefer opening File.$LANG.ext */
    MTX_CMM_EXTENSION_TABLE             = 1 << 5, /* markdown tables (monospace font) */
} MtxCmmExtensions;

typedef enum _MtxCmmTweaks
{
    MTX_CMM_TWEAK_NONE                  = 0,
    MTX_CMM_TWEAK_CM_BLOCK_END          = 1 << 0, /* add empty line at code block end */
    MTX_CMM_TWEAK_UNSAFE_HTML           = 1 << 1, /* HTML fragment includes raw HTML (cmark --unsafe) */
    MTX_CMM_TWEAK_SOFT_BREAK            = 1 << 2, /* render soft breaks as new lines (opposite of cmark --nobreaks) */
    MTX_CMM_TWEAK_HTML5                 = 1 << 3, /* instead of default XHTML */
} MtxCmmTweaks;

typedef enum _MtxCmmTagInfo
{
    MTX_TAG_DEST_LINK_URI_ID = 0,
    MTX_TAG_DEST_LINK_TXT_LEN,
    MTX_TAG_DEST_IMAGE_PATH_ID,
    MTX_TAG_BLOCKQUOTE_LEVEL,
    MTX_TAG_BLOCKQUOTE_OPEN,
    MTX_TAG_OL_UL_LEVEL,
    MTX_TAG_LI_LEVEL,
    MTX_TAG_LI_ORDINAL,
    MTX_TAG_LI_BULLET_LEN,
    MTX_TAG_LI_ID,

    /* keep last */
    MTX_TAG_INFO_LEN,
} MtxCmmTagInfo;

typedef gchar *(MtxCmmLinkBuilder)(MtxCmm *, const gchar *text, const gchar *dest, const gchar *title, const gint link_dest_id);
typedef gchar *(MtxCmmImageBuilder)(MtxCmm *, const gchar *text, const gchar *dest, const gchar *title, const gint link_dest_id);
typedef gchar *(MtxCmmAImgFormatter)(MtxCmm *, const gchar *text, const gchar *dest, const gchar *title);

typedef struct _MtxCmmTags
{
    const gchar *blockquote_start;
    const gchar *blockquote_end;
    const gchar *olist_start;
    const gchar *olist_end;
    /* alternating even/odd list bullets */
    const gchar *li_start[2];
    const gchar *li_end;
    const gchar *em_start;
    const gchar *em_end;
    const gchar *strong_start;
    const gchar *strong_end;
    const gchar *code_span_start;
    const gchar *code_span_end;
    const gchar *codeblock_start;
    const gchar *codeblock_end;
    const gchar *strikethrough_start;
    const gchar *strikethrough_end;
    const gchar *h1_start;
    const gchar *h1_end;
    const gchar *h2_start;
    const gchar *h2_end;
    const gchar *h3_start;
    const gchar *h3_end;
    const gchar *h4_start;
    const gchar *h4_end;
    const gchar *h5_start;
    const gchar *h5_end;
    const gchar *h6_start;
    const gchar *h6_end;
    const gchar *ulist_start;
    const gchar *ulist_end;
    const gchar *rule;
    const gchar *para_start;
    const gchar *para_end;
    const gchar *br;
    const gchar *table_start;
    const gchar *table_end;
    const gchar *thead_start;
    const gchar *thead_end;
    const gchar *tbody_start;
    const gchar *tbody_end;
    const gchar *tr_start;
    const gchar *tr_end;
    const gchar *th_start;
    const gchar *th_end;
    const gchar *td_start;
    const gchar *td_end;

    MtxCmmLinkBuilder *link_builder;
    MtxCmmImageBuilder *image_builder;
} MtxCmmTags;

/**
mtx_cmm_new:

Returns: a new #MTX_CMM instance.
*/
MtxCmm * mtx_cmm_new (void);

gchar *mtx_cmm_mtx (MtxCmm *, gchar **, gsize *, const gboolean);
gboolean mtx_cmm_get_render_indent (MtxCmm *);
const MtxCmmTags *mtx_cmm_get_output_tags (MtxCmm *);
MtxCmmOutput mtx_cmm_get_output (MtxCmm *);
gboolean mtx_cmm_set_output (MtxCmm *, MtxCmmOutput output);
MtxCmmExtensions mtx_cmm_get_extensions (MtxCmm *);
gboolean mtx_cmm_set_extensions (MtxCmm *, const MtxCmmExtensions);
MtxCmmTweaks mtx_cmm_get_tweaks (MtxCmm *);
gboolean mtx_cmm_set_tweaks (MtxCmm *, const MtxCmmTweaks);
gboolean mtx_cmm_get_escape (MtxCmm *);
gboolean mtx_cmm_set_escape (MtxCmm *, gboolean);
const gchar *mtx_cmm_get_link_dest (MtxCmm *, const gint link_id);
gint mtx_cmm_tag_get_info (MtxCmm *, const gchar *tag, const MtxCmmTagInfo subject);
/*
Like CommonMark cmark, by default we replace raw HTML with the comment below.
*/
#define SAFE_HTML "<!-- raw HTML omitted -->"

/*
The renderer inserts pango markup <span>s to facilitate blockquote indentation.
Spans that contain {i,s}UNIPUA_PANGO_EMPTY_SPAN exist for structural reasons
only, and the application should ultimately render them without content.
*/
#define iUNIPUA_PANGO_EMPTY_SPAN       0xF610
#define sUNIPUA_PANGO_EMPTY_SPAN       "\357\230\220"


G_END_DECLS

#endif /* MTX_CMM_H */
