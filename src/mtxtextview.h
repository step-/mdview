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
    MTX_TEXT_VIEW_SEARCH_FORE        = 0,
    MTX_TEXT_VIEW_SEARCH_BACK        = 1 << 0,
    MTX_TEXT_VIEW_SEARCH_HILIGHT     = 1 << 1,
    MTX_TEXT_VIEW_SEARCH_ONE_HILIGHT = 1 << 2,
} MtxTextViewSearchOptions;

typedef enum _MtxTextViewHilightMode
{
    MTX_TEXT_VIEW_HILIGHT_NONE = 0,
    MTX_TEXT_VIEW_HILIGHT_NORMAL,
    MTX_TEXT_VIEW_HILIGHT_SELECT,
} MtxTextViewHilightMode;

typedef struct _MtxTextViewLinkInfo
{
    /* Never store offsets! The offset of a mark *will* change,
       even if text isn't edited */
    GtkTextMark *mark;
    gint link_dest_id;
    guint llen;        /* link text width, characters */
} MtxTextViewLinkInfo;

typedef struct _MtxTextViewPrivateRendered MtxTextViewPrivateRendered;

struct _MtxTextView {
    /* TODO reorder placing public fields on top */
    /* TODO hide private fields to priv sub struct */
    GtkTextView parent;
    MtxCmm *markdown;
    gboolean hovering_over_link;
    gchar *image_directory;
    gchar **auto_languages;
    GPtrArray *link_marks;
    GtkTextBuffer *buffer;
    GtkTextTag *margin_base_tag;
    GtkTextTag *indent_base_tag;
    GtkTextTag *highlight_tag;
    MtxTextViewPrivateRendered *blockquote_start;
    MtxTextViewPrivateRendered *blockquote_end;
    guint indent_quantum;
};

struct _MtxTextViewClass
{
    GtkTextViewClass parent_class;
    void (*link_clicked)       (MtxTextView *, const gchar *uri);
    void (*hovering_over_link) (MtxTextView *, const gchar *uri);
    void (*hovering_over_text) (MtxTextView *);
    void (*file_load_complete) (MtxTextView *, const gchar *file);
    void (*curpos_changed)     (MtxTextView *, const guint curpos);
};

GtkWidget *mtx_text_view_new ();
gboolean mtx_text_view_load_file (MtxTextView *, const gchar *, const gchar *, const gboolean);
gboolean mtx_text_view_set_text (MtxTextView *, gchar **, const gchar *, const gboolean);
void mtx_text_view_reset (MtxTextView *);
void mtx_text_view_set_image_directory (MtxTextView *, const gchar *);
void mtx_text_view_set_extensions (MtxTextView *, const MtxCmmExtensions);
void mtx_text_view_set_tweaks (MtxTextView *, const MtxCmmTweaks);
gboolean mtx_text_view_find_text (MtxTextView *, const gchar *, MtxTextViewSearchOptions);
void mtx_text_view_cursor_to_top (MtxTextView *);
void mtx_text_view_set_use_gettext (MtxTextView *, const gboolean);
void mtx_text_view_set_auto_lang_open (MtxTextView *, const gboolean);
void mtx_text_view_clear_search_highlights (MtxTextView *);
void mtx_text_view_highlight_at_cursor (MtxTextView *, const MtxTextViewHilightMode);
void mtx_text_view_highlight_at_cursor_chars (MtxTextView *, const guint, const MtxTextViewHilightMode);
guint mtx_text_view_link_count (MtxTextView *);
const MtxTextViewLinkInfo *mtx_text_view_link_info_get (MtxTextView *, const guint);
const MtxTextViewLinkInfo *mtx_text_view_link_info_get_near_offset (MtxTextView *,guint, const gint);
void mtx_text_view_set_auto_lang_find (MtxTextView *, const gboolean);
gchar *mtx_text_view_auto_lang_find (MtxTextView *, const gchar *);
gchar *mtx_text_view_get_file_contents (MtxTextView *, const gchar *, gsize *, const gboolean);
gint mtx_text_view_get_link_at_iter (MtxTextView *, GtkTextIter *, const gchar **, guint *);

GType mtx_text_view_get_type();

/* Utility function */
gchar *_get_file_contents (const gchar *path, gsize *size, const gboolean);

G_END_DECLS
#endif /* __MTX_TEXT_VIEW_H__ */
