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
/*
 * Markdown Text View
 * GtkTextView subclass that supports Markdown syntax
 *
 * Copyright (C) 2009 Leandro Pereira <leandro@hardinfo.org>
 * Copyright (C) 2015 James B
 * Portions Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
 * Portions Copyright (C) GTK+ Team (based on hypertext textview demo)
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __MTX_TEXT_VIEW_H__
#define __MTX_TEXT_VIEW_H__

#include <gtk/gtk.h>
#include "mtxcmm.h"
#include "mtxcolor.h"
#include "mtxtextviewprivate.h"

G_BEGIN_DECLS
#define TYPE_MTX_TEXT_VIEW           (mtx_text_view_get_type())
#define MTX_TEXT_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_MTX_TEXT_VIEW, MtxTextView))
#define MTX_TEXT_VIEW_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST((obj), MTX_TEXT_VIEW, MtxTextViewClass))
#define IS_MTX_TEXT_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_MTX_TEXT_VIEW))
#define IS_MTX_TEXT_VIEW_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE((obj), TYPE_MTX_TEXT_VIEW))
#define MTX_TEXT_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_MTX_TEXT_VIEW, MtxTextViewClass))

typedef struct _MtxTextView MtxTextView;
typedef struct _MtxTextViewClass MtxTextViewClass;
typedef enum   _MtxTextViewSearchOptions
{
    MTX_TEXT_VIEW_SEARCH_FORE        = 1 << 0,
    MTX_TEXT_VIEW_SEARCH_BACK        = 1 << 1,
    MTX_TEXT_VIEW_SEARCH_HILIGHT     = 1 << 2,
    MTX_TEXT_VIEW_SEARCH_ONE_HILIGHT = 1 << 3,
} MtxTextViewSearchOptions;

typedef enum _MtxTextViewHilightMode
{
    MTX_TEXT_VIEW_HILIGHT_NONE       = 1 << 0,
    MTX_TEXT_VIEW_HILIGHT_NORMAL     = 1 << 1,
    MTX_TEXT_VIEW_HILIGHT_SELECT     = 1 << 2,
    MTX_TEXT_VIEW_HILIGHT_CLEAR_LINE = 1 << 3, /* on leaving curpos line */
} MtxTextViewHilightMode;

typedef enum _MtxTextViewLinkInfoType
{
    MTX_TEXT_VIEW_LINK_INFO_TYPE_NORMAL = 0,
    MTX_TEXT_VIEW_LINK_INFO_TYPE_HEADING,
} MtxTextViewLinkInfoType;

typedef struct _MtxTextViewLinkInfo
{
    /* Never store offsets! The offset of a mark *will*
       change, even if the buffer text can't be edited. */
    GtkTextMark *mark;
    const gchar *dest;              /* uri-encoded, instance-owned */
    gint dest_id;                   /* found in "font" GtkTextTag */
    guint llen;                     /* ditto, link text width, characters */
    MtxTextViewLinkInfoType type;
} MtxTextViewLinkInfo;

typedef struct _MtxTextViewLinkTag
{
    /* all member values filled from the "font" GtkTextTag */
    gint dest_id;
    guint llen;
    MtxTextViewLinkInfoType type;
} MtxTextViewLinkTag;

typedef enum _MtxTextViewProgress
{
    MTX_TEXT_VIEW_PROGRESS_START = MTX_CMM_PROGRESS_END + 1,
    MTX_TEXT_VIEW_PROGRESS_MARKUP_INSERTED,
    MTX_TEXT_VIEW_PROGRESS_IMAGES_LINKS,
    MTX_TEXT_VIEW_PROGRESS_INDENTED,
    MTX_TEXT_VIEW_PROGRESS_RENDERED,
    MTX_TEXT_VIEW_PROGRESS_END,
} MtxTextViewProgress;

typedef struct _MtxTextViewPrivateRendered MtxTextViewPrivateRendered;

struct _MtxTextView {
    GtkTextView parent;
    MtxCmm *markdown;
    gboolean hovering_over_link;
    gchar *image_directory;
    gchar **auto_languages;
    GPtrArray *link_marks;
    GHashTable *link_dests, *link_dests_loading;
    GtkTextBuffer *buffer;
    PangoLayout *make_indent;
    GtkTextTag *highlight_tag;
    MtxTextViewPrivateRendered *blockquote_start;
    MtxTextViewPrivateRendered *blockquote_end;
    guint indent_quantum;
    guint indent_chwidth;
    GtkTextMark *jumpoff_mark;
    GCancellable *load_markup_cancellable;
    MtxCmmPageMeta *page_meta;
    gchar *page_toc;
    gint progress_fd;
};

struct _MtxTextViewClass
{
    GtkTextViewClass parent_class;
    void (*link_clicked)       (MtxTextView *, const gchar *uri);
    void (*hovering_over_link) (MtxTextView *, const gchar *uri);
    void (*hovering_over_text) (MtxTextView *);
    void (*file_load_complete) (MtxTextView *, const gchar *file);
    void (*new_text_buffer)    (MtxTextView *, const gpointer);
    void (*curpos_changed)     (MtxTextView *, const guint curpos);
};

/***********************************************************/

GtkWidget *
mtx_text_view_new();

MtxTextViewLinkTag *
mtx_text_view_get_link_tag_at_iter (MtxTextView *self,
                                    GtkTextIter *iter);

const MtxTextViewLinkInfo *
mtx_text_view_get_link_info_from_link_tag (MtxTextView *self,
                                           const MtxTextViewLinkTag *link_tag);

guint
mtx_text_view_link_count (MtxTextView *self);

const MtxTextViewLinkInfo *
mtx_text_view_link_info_get (MtxTextView *self,
                             const guint index_);

const MtxTextViewLinkInfo *
mtx_text_view_link_info_get_near_offset (MtxTextView *self,
                                         guint offset,
                                         const gint direction);

gboolean
mtx_text_view_set_text (MtxTextView *self,
                        gchar **text,
                        const gchar *referrer,
                        const gboolean clear_text, /* free *text and text */
                        const gchar *file_complete,
                        GClosure *completer);

void
mtx_text_view_load_markup_cancel (MtxTextView *self);

gboolean
mtx_text_view_load_file (MtxTextView *self,
                         const gchar *file,
                         const gchar *referrer,
                         const gboolean utf8_validate,
                         GClosure *completer);

void
mtx_text_view_set_image_directory (MtxTextView *self,
                                   const gchar *directory);

void
mtx_text_view_set_auto_lang_find (MtxTextView *self,
                                  const gboolean enable);

void
mtx_text_view_set_extensions (MtxTextView *self,
                              const MtxCmmExtensions flags);

gboolean
mtx_text_view_set_progress_fd (MtxTextView *self,
                               const gint fd);

const GRegex *
mtx_text_view_get_regex_astx (MtxTextView *self);

guint
mtx_text_view_get_toc_level (MtxTextView *self);

void
mtx_text_view_set_toc_level (MtxTextView *self,
                             const guint value);

void
mtx_text_view_set_tweaks (MtxTextView *self,
                          const MtxCmmTweaks flags);

const MtxCmmPageMeta *
mtx_text_view_fetch_page_meta (MtxTextView *self);

const gchar *
mtx_text_view_fetch_page_toc_md (MtxTextView *self);

void
mtx_text_view_clear_line_highlights (MtxTextView *self,
                                     GtkTextMark *mark);

void
mtx_text_view_clear_page_highlights (MtxTextView *self);

gboolean
mtx_text_view_find_text (MtxTextView *self,
                         const gchar *search_text,
                         MtxTextViewSearchOptions options);

void
mtx_text_view_highlight_at_cursor (MtxTextView *self,
                                   const MtxTextViewHilightMode mode);

void
mtx_text_view_highlight_at_cursor_chars (MtxTextView *self,
                                         const guint chars,
                                         const MtxTextViewHilightMode mode);

void
mtx_text_view_cursor_to_top (MtxTextView *self);

gchar *
mtx_text_view_mmap_read_file (const gchar *path,
                              gsize *size,
                              const gboolean utf8_validate);

gchar *
mtx_text_view_auto_lang_find (MtxTextView *self,
                              const gchar *path);

gchar *
mtx_text_view_get_file_contents (MtxTextView *self,
                                 const gchar *path,
                                 gsize *size,
                                 const gboolean utf8_validate);

/***********************************************************/

GType mtx_text_view_get_type();

G_END_DECLS
#endif /* __MTX_TEXT_VIEW_H__ */
