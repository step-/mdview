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

#ifndef MTX_CMM_H
#define MTX_CMM_H

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MTX_TYPE_CMM         (mtx_cmm_get_type())
G_DECLARE_FINAL_TYPE (MtxCmm, mtx_cmm, MTX, CMM, GObject)

/**********************************************************************/

/**********************************************************************/


typedef enum _MtxCmmOutput
{
    MTX_CMM_OUTPUT_UNKNOWN              = 0,
    MTX_CMM_OUTPUT_ANSI                 = 1 << 0,
    MTX_CMM_OUTPUT_TTY                  = 1 << 1,
    MTX_CMM_OUTPUT_TEXT                 = 1 << 2,
    MTX_CMM_OUTPUT_PANGO                = 1 << 3,
    MTX_CMM_OUTPUT_HTML                 = 1 << 4,
    MTX_CMM_OUTPUT_BARE_INLINE          = 1 << 5, /* mtx_insert_heading_link_cb */
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
    MTX_CMM_EXTENSION_HEADING_LINK      = 1 << 6, /* auto heading reference links */
    MTX_CMM_EXTENSION_STRIKETHROUGH     = 1 << 7, /* ~strike~ and ~~strike~~ */
} MtxCmmExtensions;

#define MTX_CMM_EXTENSION_DEFAULT (\
    MTX_CMM_EXTENSION_SHEBANG | \
    MTX_CMM_EXTENSION_SMART_TEXT | \
    MTX_CMM_EXTENSION_AUTO_CODE | \
    MTX_CMM_EXTENSION_PERMLINK | \
    MTX_CMM_EXTENSION_TABLE | \
    MTX_CMM_EXTENSION_HEADING_LINK | \
    MTX_CMM_EXTENSION_STRIKETHROUGH)

typedef enum _MtxCmmTweaks
{
    MTX_CMM_TWEAK_NONE                  = 0,
    MTX_CMM_TWEAK_CM_BLOCK_END          = 1 << 0, /* add empty line at code block end */
    MTX_CMM_TWEAK_UNSAFE_HTML           = 1 << 1, /* HTML fragment includes raw HTML (cmark --unsafe) */
    MTX_CMM_TWEAK_SOFT_BREAK            = 1 << 2, /* render soft breaks as line endings (all output modes) */
    MTX_CMM_TWEAK_SOFT_BREAK_BR         = 1 << 3, /* as above + insert <br> before line ending (HTML mode) */
    MTX_CMM_TWEAK_HTML5                 = 1 << 4, /* output HTML 5 instead of the default XHTML */
    MTX_CMM_TWEAK_FULL_HTML             = 1 << 5, /* generate full HTML including styles */
    MTX_CMM_TWEAK_RESERVED1             = 1 << 6, /* --pango */
    MTX_CMM_TWEAK_RESERVED2             = 1 << 7, /* --exit-test */
    MTX_CMM_TWEAK_RESERVED3             = 1 << 8, /* .md$ => .md.html href */
    MTX_CMM_TWEAK_RESERVED4             = 1 << 9, /* --lint */
} MtxCmmTweaks;

typedef enum _MtxCmmTagInfo
{
    MTX_TAG_DEST_LINK_URI_ID = 0,
    MTX_TAG_DEST_LINK_TXT_LEN,
    MTX_TAG_DEST_LINK_HEADING,
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
    const gchar *toc_start;
    const gchar *toc_end;
    MtxCmmLinkBuilder *link_builder;
    MtxCmmImageBuilder *image_builder;
} MtxCmmTags;

typedef struct _MtxCmmPageMeta
{
    gint renderer_keep_tags;
    gint renderer_skip_toc;
    gint viewer_track_page;
} MtxCmmPageMeta;

typedef enum _MtxCmmProgress
{
    MTX_CMM_PROGRESS_START = 0,
    MTX_CMM_PROGRESS_SHEBANG,
    MTX_CMM_PROGRESS_LEGACY,
    MTX_CMM_PROGRESS_HEADINGS,
    MTX_CMM_PROGRESS_PARSED,
    MTX_CMM_PROGRESS_CONSOLIDATED,
    MTX_CMM_PROGRESS_COLLAPSED,
    MTX_CMM_PROGRESS_ELIDED,
    MTX_CMM_PROGRESS_TABLE_PREPROCESSED,
    MTX_CMM_PROGRESS_TABLE_JUSTIFIED,
    MTX_CMM_PROGRESS_TEXT_TRANSFORMED,
    MTX_CMM_PROGRESS_JOINED,
    MTX_CMM_PROGRESS_TOC,
    MTX_CMM_PROGRESS_END,
} MtxCmmProgress;

/***********************************************************/

MtxCmm *
mtx_cmm_new (MtxCmmOutput output);

gint
mtx_cmm_get_tag_val (MtxCmm *self,
                     const gchar *tag,
                     const MtxCmmTagInfo subject);

const gchar *
mtx_cmm_get_link_dest (MtxCmm *self,
                       const gint id);

gboolean
mtx_cmm_get_render_indent (MtxCmm *self);

gboolean
mtx_cmm_get_escape (MtxCmm *self);

gboolean
mtx_cmm_set_escape (MtxCmm *self,
                    gboolean escape);

MtxCmmExtensions
mtx_cmm_get_extensions (MtxCmm *self);

gboolean
mtx_cmm_set_extensions (MtxCmm *self,
                        const MtxCmmExtensions flags);

gboolean
mtx_cmm_set_progress_fd (MtxCmm *self,
                         const gint fd);

guint
mtx_cmm_get_toc_level (MtxCmm *self);

gboolean
mtx_cmm_set_toc_level (MtxCmm *self,
                       const guint value);

MtxCmmTweaks
mtx_cmm_get_tweaks (MtxCmm *self);

gboolean
mtx_cmm_set_tweaks (MtxCmm *self,
                    const MtxCmmTweaks flags);

const MtxCmmTags *
mtx_cmm_get_output_tags (MtxCmm *self);

MtxCmmOutput
mtx_cmm_get_output (MtxCmm *self);

gboolean
mtx_cmm_set_output (MtxCmm *self,
                    MtxCmmOutput output);

gboolean
mtx_cmm_got_blockquote (MtxCmm *self);

gboolean
mtx_cmm_got_img (MtxCmm *self);

gboolean
mtx_cmm_got_li (MtxCmm *self);

gboolean
mtx_cmm_got_link (MtxCmm *self);

GRegex *
mtx_cmm_regex_astx (MtxCmm *self);

gchar *
mtx_cmm_mtx (MtxCmm *self,
             gchar **markdown,
             gsize *size,
             MtxCmmPageMeta **meta,
             gchar **rtoc,
             const gboolean clear_markdown,
             GCancellable *cancellable);

/***********************************************************/

/*
The renderer inserts pango markup <span>s to facilitate blockquote indentation.
Spans that contain {i,s}UNIPUA_PANGO_EMPTY_SPAN exist for structural reasons
only, and the application should ultimately render them without content.
*/
#define iUNIPUA_PANGO_EMPTY_SPAN       0xF610
#define sUNIPUA_PANGO_EMPTY_SPAN       "\357\230\220"


G_END_DECLS

#endif /* MTX_CMM_H */
