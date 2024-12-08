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
2024-08-15 step:
This file was derived from mdview3 markdown-text-view.c with substantial changes
and feature additions for MTX.
*/
/*
 * Markdown Text View
 * GtkTextView subclass that supports Markdown syntax
 *
 * Copyright (C) 2009 Leandro Pereira <leandro@hardinfo.org>
 * Copyright (C) 2015 James B
 * Copyright (C) 2023 step
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "mtxtextview.h"
#include "mtxdbg.h"

#define MTX_TEXT_VIEW_LEFT_MARGIN           10
#define MTX_TEXT_VIEW_RIGHT_MARGIN          10
#define MTX_TEXT_VIEW_PIXEL_ABOVE_LINE       3
#define MTX_TEXT_VIEW_PIXEL_BELOW_LINE       3

static GdkCursor *hand_cursor = NULL;
static GdkScreen *screen = NULL;
static GdkDisplay *display = NULL;
#ifdef MTX_TEXT_VIEW_DEBUG
gchar *gl_text_view_debug_markup_file = NULL;
#endif

G_DEFINE_TYPE(MtxTextView, mtx_text_view, GTK_TYPE_TEXT_VIEW);

enum {
     LINK_CLICKED,
     HOVERING_OVER_LINK,
     HOVERING_OVER_TEXT,
     FILE_LOAD_COMPLETE,
     LAST_SIGNAL
};

static guint mtx_text_view_signals[LAST_SIGNAL] = { 0 };

GtkWidget *mtx_text_view_new()
{
    return g_object_new(TYPE_MTX_TEXT_VIEW, NULL);
}

static void mtx_text_view_dispose (GObject *object);
static void mtx_text_view_finalize (GObject *object);

static void mtx_text_view_class_init (MtxTextViewClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = mtx_text_view_dispose;
    object_class->finalize = mtx_text_view_finalize;

    screen = gdk_screen_get_default ();
    display = gdk_screen_get_display (screen);
    if (!hand_cursor)
    {
        hand_cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
    }
    mtx_text_view_signals[LINK_CLICKED] =
    g_signal_new ("link-clicked", G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET
                  (MtxTextViewClass, link_clicked), NULL, NULL,
                  g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
                  G_TYPE_STRING);
    mtx_text_view_signals[HOVERING_OVER_LINK] =
    g_signal_new ("hovering-over-link", G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MtxTextViewClass,
                                   hovering_over_link), NULL, NULL,
                  g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
                  G_TYPE_STRING);
    mtx_text_view_signals[HOVERING_OVER_TEXT] =
    g_signal_new ("hovering-over-text", G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MtxTextViewClass,
                                   hovering_over_text), NULL, NULL,
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    mtx_text_view_signals[FILE_LOAD_COMPLETE] =
    g_signal_new ("file-load-complete", G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MtxTextViewClass,
                                   file_load_complete), NULL, NULL,
                  g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

/**
_text_buffer_insert_markup:
*/
/*
2024-08-08 step:
The code in this function was modified from an original patch that
Tim-Philipp Müller proposed to the GTK maintainers for GTK 2.6 in 2004,
https://bugzilla.gnome.org/show_bug.cgi?id=59390.  The patch was never merged
but in 2014 it made its way into GTK3's gtk_text_buffer_insert_markup(),
https://bugzilla.gnome.org/show_bug.cgi?id=59390#c28.
Let's give credit where credit's due.
LIMITATION:
This code, as well as GTK3's own gtk_text_buffer_insert_markup and the original
patch, all cannot render the following markup correctly:
    <span font="monospace"><i>em</i> and <b>strong</b></span>
indeed <i> and <b> are not rendered. You can easily double-check GTK3 with
    yadu --formatted --text-info <<< 'insert the test markup above'

Interestingly, GtkLabel _can_ render the markup correctly. Check with
    yad --text='insert the test markup above'
This works for GTK2 and GTK3. However, their code is hefty and outside the API.
So it becomes essential for our renderer not to embed Pango style tags in
in an outer <span font="..."> otherwise the viewer won't render the styles.
*/
static void
_text_buffer_insert_markup (GtkTextBuffer *buffer,
                            GtkTextIter *iter,
                            const gchar *markup)
{
    PangoAttrIterator *paiter;
    PangoAttrList *attrlist;
    GtkTextTagTable *tags;
    GtkTextMark *mark;
    GError *error = NULL;
    gchar *text;

    g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (markup != NULL);
    if (markup[0] == '\0')
    {
        return;
    }
    if (!pango_parse_markup (markup, -1, 0, &attrlist, &text, NULL, &error))
    {
        g_warning ("Invalid markup string: %s", error->message);
        g_error_free (error);
        return;
    }
    /* text without markup */
    if (attrlist == NULL)
    {
        gtk_text_buffer_insert (buffer, iter, text, -1);
        g_free (text);
        return;
    }

    mark = gtk_text_buffer_create_mark (buffer, NULL, iter, FALSE);
    paiter = pango_attr_list_get_iterator (attrlist);
    tags = gtk_text_buffer_get_tag_table (buffer);
    do
    {
        gint start, end, ival;
        PangoAttribute *attr;
        GtkTextTag *tag = gtk_text_tag_new (NULL);

        pango_attr_iterator_range (paiter, &start, &end);
        if (end == G_MAXINT)
        {
            end = start - 1; /* chunk size > max signed int */
        }
        /* https://docs.gtk.org/Pango/enum.AttrType.html */

#if MTX_TEXT_VIEW_DEBUG > 0
        fprintf (stderr, "%s (", __FUNCTION__);
        for (gint i = start; i < end; i++)
        {
            fputc (text[i], stderr);
        }
        fputc (')', stderr);
#endif
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_LANGUAGE)))
        {
            g_object_set (tag, "language",
                          pango_language_to_string (((PangoAttrLanguage *)
                                                     attr)->value), NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_FAMILY)))
        {
            g_object_set (tag, "family", ((PangoAttrString *) attr)->value,
                          NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_STYLE)))
        {
            ival = ((PangoAttrInt *) attr)->value;
            g_object_set (tag, "style", ival, NULL);
#if MTX_TEXT_VIEW_DEBUG > 0
            fprintf (stderr, " style %d", ival);
#endif
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_WEIGHT)))
        {
            ival = ((PangoAttrInt *) attr)->value;
            g_object_set (tag, "weight", ival, NULL);
#if MTX_TEXT_VIEW_DEBUG > 0
            fprintf (stderr, " weight %d", ival);
#endif
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_VARIANT)))
        {
            g_object_set (tag, "variant", ((PangoAttrInt *) attr)->value, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_STRETCH)))
        {
            g_object_set (tag, "stretch", ((PangoAttrInt *) attr)->value, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_SIZE)))
        {
            ival = ((PangoAttrInt *) attr)->value;
            g_object_set (tag, "size", ival, NULL);
#if MTX_TEXT_VIEW_DEBUG > 0
            fprintf (stderr, " size %d", ival);
#endif
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_FONT_DESC)))
        {
            g_object_set (tag, "font-desc",
                          ((PangoAttrFontDesc *) attr)->desc, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_FOREGROUND)))
        {
            GdkColor col = { 0,
                ((PangoAttrColor *) attr)->color.red,
                ((PangoAttrColor *) attr)->color.green,
                ((PangoAttrColor *) attr)->color.blue
            };
            g_object_set (tag, "foreground-gdk", &col, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_BACKGROUND)))
        {
            GdkColor col = { 0,
                ((PangoAttrColor *) attr)->color.red,
                ((PangoAttrColor *) attr)->color.green,
                ((PangoAttrColor *) attr)->color.blue
            };
            g_object_set (tag, "background-gdk", &col, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_UNDERLINE)))
        {
            ival = ((PangoAttrInt *) attr)->value;
            g_object_set (tag, "underline", ival, NULL);
#if MTX_TEXT_VIEW_DEBUG > 0
            fprintf (stderr, " underline %d", ival);
#endif
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_STRIKETHROUGH)))
        {
            g_object_set (tag, "strikethrough",
                          (gboolean) (((PangoAttrInt *) attr)->value != 0),
                          NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_RISE)))
        {
            g_object_set (tag, "rise", ((PangoAttrInt *) attr)->value, NULL);
        }
        if ((attr = pango_attr_iterator_get (paiter, PANGO_ATTR_SCALE)))
        {
            g_object_set (tag, "scale", ((PangoAttrFloat *) attr)->value, NULL);
        }
        /* TODO: add attributes underline-gdk (?-rgba), strikethrough-gdk (?-rgba)
           fallback (int), letter-spacing (int), font-features (string) */
#if MTX_TEXT_VIEW_DEBUG > 0
        {
            gchar *font = NULL;
            g_object_get (G_OBJECT (tag), "font", &font, NULL);
            fprintf (stderr, " font %s", font);
            g_free (font);
        }
        fputc ('\n', stderr);
#endif
        gtk_text_tag_table_add (tags, tag);
        gtk_text_buffer_insert_with_tags (buffer, iter, text + start,
                                          end - start, tag, NULL);
        g_object_unref (tag);
        /* mark moves right after appended text */
        gtk_text_buffer_get_iter_at_mark (buffer, iter, mark);
    }
    while (pango_attr_iterator_next (paiter));

    gtk_text_buffer_delete_mark (buffer, mark);
    pango_attr_iterator_destroy (paiter);
    pango_attr_list_unref (attrlist);
    g_free (text);
}

/**
mtx_text_view_get_link_at_iter:

@self:
@iter:
@link_dest: pointer to a return destionation for the link destination. NULLABLE.
@length: pointer to a return destination for the link text length (in
characters). NULLABLE.

Returns: `link_dest_id` > 0 if @iter is somewhere inside a link text span, and
sets *@length to the length (in characters) of the link text. Otherwise it
returns -1 and *@length is unchanged. Note that a successful return value
does not imply that @iter is at the start of the link text span.
*/
gint
mtx_text_view_get_link_at_iter (MtxTextView *self,
                                GtkTextIter *iter,
                                const gchar **link_dest,
                                guint *length)
{
    GSList *tags = NULL, *tagp = NULL;
    gint id = -1;

    tags = gtk_text_iter_get_tags (iter);
    for (tagp = tags; tagp != NULL; tagp = tagp->next)
    {
        GtkTextTag *tag = tagp->data;
        gchar *font = NULL;
        g_object_get (G_OBJECT (tag), "font", &font, NULL);
        /* prereq for (concat of) MTX_TAG_DEST_* */
        if (strstr (font, "@dest=") == NULL)
        {
            g_free (font);
            continue;
        }
        id = mtx_cmm_tag_get_info (self->markdown, font,
                                   MTX_TAG_DEST_LINK_URI_ID);
        if (id >= 0)
        {
            if (length)
            {
                *length = mtx_cmm_tag_get_info (self->markdown, font,
                                                MTX_TAG_DEST_LINK_TXT_LEN);
            }
            if (link_dest)
            {
                *link_dest = mtx_cmm_get_link_dest (self->markdown, id);
            }
            g_free (font);
            break;
        }
    }
    if (tags != NULL)
    {
        g_slist_free (tags);
    }
    return id;
}

/**
set_cursor_and_signal_on_hover:
*/
static void
set_cursor_and_signal_on_hover (MtxTextView *self,
                                gint x,
                                gint y)
{
    GtkTextIter iter;
    gboolean hovering = FALSE;
    const gchar *link_dest = NULL;

    gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (self), &iter, x, y);
    (void) mtx_text_view_get_link_at_iter (self, &iter, &link_dest, NULL);
    hovering = link_dest != NULL;
    if (hovering != self->hovering_over_link)
    {
        self->hovering_over_link = hovering;
        if (self->hovering_over_link)
        {
            /* link_dest format: <uri-encoded>\n<verbatim> */
            g_signal_emit (self, mtx_text_view_signals[HOVERING_OVER_LINK],
                           0, /* verbatim */ strchr (link_dest, '\n') + 1);
            gdk_window_set_cursor (gtk_text_view_get_window
                                   (GTK_TEXT_VIEW (self),
                                    GTK_TEXT_WINDOW_TEXT), hand_cursor);
        }
        else
        {
            g_signal_emit (self, mtx_text_view_signals[HOVERING_OVER_TEXT], 0);
            gdk_window_set_cursor (gtk_text_view_get_window
                                   (GTK_TEXT_VIEW (self), GTK_TEXT_WINDOW_TEXT),
                                   NULL);
        }
    }
}

/**
motion_notify_event:
Update the cursor image while the pointer moves.

Side effects: if the pointer is clicked over links, a link-navigation signal
will be emitted.
 */
static gboolean
motion_notify_event (GtkWidget *self,
                     GdkEventMotion *event)
{
    gint x, y;

    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (self),
                                           GTK_TEXT_WINDOW_WIDGET,
                                           event->x, event->y, &x, &y);
    set_cursor_and_signal_on_hover (MTX_TEXT_VIEW (self), x, y);
#if !GTK_CHECK_VERSION(3,0,0)
    gdk_window_get_pointer (gtk_widget_get_window (self), NULL, NULL, NULL);
#endif
    return FALSE;
}

/*
visibility_notify_event:
Also update the cursor image if the window becomes visible, e.g.
another window obscuring our window is iconified.
*/
static gboolean
visibility_notify_event (GtkWidget *self,
                         GdkEventVisibility *event __attribute__((unused)))
{
    gint wx = 0, wy = 0, bx, by;

#if !GTK_CHECK_VERSION(3,0,0)
    gdk_window_get_pointer (gtk_widget_get_window (self), &wx, &wy, NULL);
#else
    GdkDevice *device = NULL, *pointer_device = NULL;
    device = gdk_event_get_device ((GdkEvent *) event);
    if (device)
    {
        pointer_device = gdk_device_get_associated_device (device);
    }
    if (pointer_device)
    {
        gdk_device_get_position (pointer_device, NULL, &wx, &wy);
    }
#endif
    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (self),
                                           GTK_TEXT_WINDOW_WIDGET,
                                           wx, wy, &bx, &by);
    set_cursor_and_signal_on_hover (MTX_TEXT_VIEW (self), bx, by);
    return FALSE;
}

/**
follow_if_link:
*/
static void
follow_if_link (GtkWidget *widget,
                GtkTextIter *iter)
{
    MtxTextView *self = MTX_TEXT_VIEW (widget);
    const gchar *link_dest = NULL;

    (void) mtx_text_view_get_link_at_iter (self, iter, &link_dest, NULL);

    if (link_dest != NULL)
    {
        /* link_dest format: <uri-encoded>\n<verbatim> */
        g_signal_emit (self, mtx_text_view_signals[LINK_CLICKED], 0, link_dest);
    }
}

/**
_text_buffer_get_cursor:
@buffer:
@iter: #GtkTextIter at cursor position, returned.
*/
inline static void
_text_buffer_get_cursor (GtkTextBuffer *buffer,
                         GtkTextIter *iter)
{
    GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, iter, mark);
    /* the 'insert' mark is permanent */
}

static gboolean
key_press_event (GtkWidget *self,
                 GdkEventKey *event)
{
    GtkTextIter iter;

    switch (event->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        _text_buffer_get_cursor ((MTX_TEXT_VIEW (self))->buffer, &iter);
        follow_if_link (self, &iter);
        break;
    default:
        break;
    }
    return FALSE;
}

static gboolean
event_after (GtkWidget *self,
             GdkEvent *ev)
{
    GtkTextIter start, end, iter;
    GdkEventButton *event;
    gint x, y;

    if (ev->type != GDK_BUTTON_RELEASE)
    {
        return FALSE;
    }
    event = (GdkEventButton *) ev;
    if (event->button != 1)
    {
        return FALSE;
    }
    gtk_text_buffer_get_selection_bounds ((MTX_TEXT_VIEW (self))->buffer,
                                          &start, &end);
    if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
    {
        return FALSE;
    }
    /* we get here iff event == button 1 released && nothing's selected */

    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (self),
                                           GTK_TEXT_WINDOW_WIDGET,
                                           event->x, event->y, &x, &y);
    gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (self), &iter, x, y);
    follow_if_link (self, &iter);
    return FALSE;
}

/**
*/
void
_text_tag_table_remove_foreach (GtkTextTag *tag,
                               gpointer *data)
{
    gchar *name;
    g_object_get (tag, "name", &name, NULL);
    if (name == NULL)
    {
        gtk_text_tag_table_remove ((GtkTextTagTable *) data, tag);
    }
    g_free (name);
}

/**
mtx_text_view_reset:
Reset the text buffer, and remove all anonymous tags for which the application
doesn't hold a reference.
*/
void
mtx_text_view_reset (MtxTextView *self)
{
    g_return_if_fail (IS_MTX_TEXT_VIEW (self));

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table (self->buffer);
    gtk_text_tag_table_foreach (table,
                                (GtkTextTagTableForeach)
                                _text_tag_table_remove_foreach, table);
    gtk_text_buffer_set_text (self->buffer, "\n", 1);
}

/**
get_pixbuf_from_id:
Get an image pixbuf for a link_dest id.

@referrer: absolute pathname (not necessarily directory) to fall back to when
resolving the image path. NULLABLE.

Return: pointer to #GdkPixbuf or NULL if no image can be found.
*/
static GdkPixbuf *
get_pixbuf_from_id (MtxTextView *self,
                    const gint id,
                    const gchar *referrer)
{
    GdkPixbuf *pixbuf = NULL;
    const gchar *link_dest;
    gchar *path, *pi = NULL;

    if (id < 0)
    {
        return NULL;
    }
    link_dest = mtx_cmm_get_link_dest (self->markdown, id);
    /* link_dest format: <uri-encoded>\n<verbatim> */
    /* get <verbatim> in directory context */
    path = g_build_filename (self->image_directory,
                             strchr (link_dest, '\n') + 1, NULL);
    if (self->auto_languages != NULL)
    {
        pi = mtx_text_view_auto_lang_find (self, path);
    }
    pixbuf = gdk_pixbuf_new_from_file (pi ? pi : path, NULL);
    if (pixbuf == NULL && referrer != NULL)
    {
        /* retry relative to referrer's directory */
        gchar *dir = g_path_get_dirname (referrer);
        g_free (path);
        path = g_build_filename (dir, strchr (link_dest, '\n') + 1, NULL);
        g_free (pi);
        pi = NULL;
        if (self->auto_languages != NULL)
        {
            pi = mtx_text_view_auto_lang_find (self, path);
        }
        g_free (dir);
        pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    }
    g_free (pi);
    g_free (path);
    return pixbuf;
}

/**
*/
static void
_clear_link_marks_el (gpointer el, gpointer user_data)
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER (user_data);
    GtkTextMark *mark = ((MtxTextViewLinkInfo *) el)->mark;
    gtk_text_buffer_delete_mark (buffer, mark);
    g_free (el);
}

/**
mtx_text_view_link_count:

Returns: the number of markdown links in the page.
*/
guint
mtx_text_view_link_count (MtxTextView *self)
{
    return self->link_marks->len;
}

/**
mtx_text_view_link_info_get:

Returns: a pointer to the #MtxTextView structure at the given index or NULL
for an out-of-range index.  The instance owns the returned memory.
*/
const MtxTextViewLinkInfo *
mtx_text_view_link_info_get (MtxTextView *self,
                             const guint index_)
{
    return g_ptr_array_index (self->link_marks, index_);
}

/**
mtx_text_view_link_info_get_near_offset:

@offset: %MtxTextView buffer offset.
@direction: -1 backward/before the offset; 1 forward/after the offset.

If @direction is -1 and the offset is before the first %MtxTextViewLinkInfo in
the buffer, the function returns the last %MtxTextViewLinkInfo in the buffer.
If @direction is 1 and the offset is after the last %MtxTextViewLinkInfo in the
buffer, the function returns the first %MtxTextViewLinkInfo in the buffer.

Returns: a pointer to the #MtxTextViewLinkInfo structure near the given offset
or NULL if there are no structures.  The instance owns the returned memory.
*/
const MtxTextViewLinkInfo *
mtx_text_view_link_info_get_near_offset (MtxTextView *self,
                                         guint offset,
                                         const gint direction)
{
    MtxTextViewLinkInfo *p;
    gint i;
    guint off;
    gboolean found = FALSE;
    GtkTextIter it;

    if (self->link_marks->len == 0)
    {
        return NULL;
    }
    if (direction > 0)
    {
        i = -1;
        do {
            i++;
            p = (MtxTextViewLinkInfo *) g_ptr_array_index (self->link_marks, i);
            gtk_text_buffer_get_iter_at_mark (self->buffer, &it, p->mark);
            off = gtk_text_iter_get_offset (&it);
            if (off > offset)
            {
                found = TRUE;
            }
        } while (!found && i < (int) self->link_marks->len - 1);
        if (!found)
        {
            p = (MtxTextViewLinkInfo *) g_ptr_array_index (self->link_marks, 0);
        }
    }
    else
    {
        i = self->link_marks->len;
        do {
            i--;
            p = (MtxTextViewLinkInfo *) g_ptr_array_index (self->link_marks, i);
            gtk_text_buffer_get_iter_at_mark (self->buffer, &it, p->mark);
            off = gtk_text_iter_get_offset (&it);
            if (off < offset)
            {
                found = TRUE;
            }
        } while (!found && i > 0);
        if (!found)
        {
            p = (MtxTextViewLinkInfo *)
            g_ptr_array_index (self->link_marks, self->link_marks->len - 1);
        }
    }
    return p;
}

/**
load_images_and_mark_links:
Resolve all image text tags in the text view.
Also save the position of markdown link text tags.

@self:
@referrer: absolute pathname (not necessarily directory) to fall back to when
resolving image paths. NULLABLE.
*/
static void
load_images_and_mark_links (MtxTextView *self,
                            const gchar *referrer)
{
    if (referrer != NULL)
    {
        g_return_if_fail (g_path_is_absolute (referrer));
    }
    GtkTextIter iter;
    GSList *tags, *tagp;

    if (self->link_marks->len > 0)
    {
        g_ptr_array_foreach (self->link_marks, _clear_link_marks_el,
                             self->buffer);
        g_ptr_array_set_size (self->link_marks, 0);
    }
    gtk_text_buffer_get_start_iter (self->buffer, &iter);
    do
    {
        gboolean done = FALSE;
        gint forward_chars = -1;

        tags = gtk_text_iter_get_tags (&iter);
        for (tagp = tags; tagp != NULL && !done; tagp = tagp->next)
        {
            GtkTextTag *tag = tagp->data;
            gchar *font = NULL;
            MtxTextViewLinkInfo *link_info = NULL;
            gint id;

            /*
            Markdown links and images piggyback the font tag.

            Markdown link text can embed an image, e.g.
              [![image of the sun](sun.jpg)](https://sun.stars.org)
            which yields the following pango markup (all on one line):
              <span font="@dest=Lu1dest=Tl5" fgcolor="#048" underline="single">
              <span font="@dest=Ip0" underline="double">image of the sun</span>
              </span>
            Conversely, markdown image alt text can embed a link, e.g.
              ![[link to stars.org](https://sun.stars.org)](sun.jpg)
            but the link is flattened, yielding the following pango markup:
              <span font="@dest=Ip0" underline="double">link to stars.org</span>

            Therefore,
            we need to process links and images independently of each other;
            and we need to process link tags before image tags.
            */
            g_object_get (G_OBJECT (tag), "font", &font, NULL);

            id = mtx_cmm_tag_get_info (self->markdown, font,
                                            MTX_TAG_DEST_LINK_URI_ID);
            if (id >= 0)            /* Markdown link text.            */
            {
                /* Save a text mark for link navigation. */
                link_info = g_malloc (sizeof (MtxTextViewLinkInfo));
                link_info->mark =
                gtk_text_buffer_create_mark (self->buffer, NULL, &iter, TRUE);
                link_info->link_dest_id = id;
                link_info->llen =
                mtx_cmm_tag_get_info (self->markdown, font,
                                      MTX_TAG_DEST_LINK_TXT_LEN);
                g_ptr_array_add (self->link_marks, link_info);

                /* Skip over contained tags, which would beget
                   bogus link navigation stops. */
                forward_chars = link_info->llen;

                done = TRUE;
            }

            id = mtx_cmm_tag_get_info (self->markdown, font,
                                       MTX_TAG_DEST_IMAGE_PATH_ID);
            GdkPixbuf *pixbuf = get_pixbuf_from_id (self, id, referrer);
            if (pixbuf != NULL)         /* Markdown image.           */
            {                           /* Replace text with pixbuf. */
                GtkTextMark *mark;
                GtkTextIter start;

                if (forward_chars < 0)
                {
                    mark =
                    gtk_text_buffer_create_mark (self->buffer, NULL, &iter,
                                                 FALSE);
                    start = iter;
                    /* Image isn't embedded in markdown link text. */
                    gtk_text_iter_forward_to_tag_toggle (&iter, tag);
                    gtk_text_buffer_delete (self->buffer, &start, &iter);
                    gtk_text_buffer_insert_pixbuf (self->buffer, &iter, pixbuf);
                }
                else
                {
                    /* Image is embedded in a link so set up hover target. */
                    /* TODO support image + text, e.g.
                       [Our star the Sun ![alt](sun.png)](stars.md) */

                    mark =
                    gtk_text_buffer_create_mark (self->buffer, NULL, &iter,
                                                 TRUE);
                    gtk_text_buffer_insert (self->buffer, &iter, " ", 1);
                    gtk_text_iter_forward_to_tag_toggle (&iter, tag);
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &start,
                                                      mark);
                    gtk_text_iter_forward_chars (&start, 1);
                    gtk_text_buffer_delete (self->buffer, &start, &iter);
                    gtk_text_buffer_insert_pixbuf (self->buffer, &iter, pixbuf);
                    gtk_text_buffer_insert (self->buffer, &iter, " ", 1);
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &start,
                                                      mark);
                    gtk_text_buffer_apply_tag (self->buffer, tag, &start,
                                               &iter);
                    link_info->llen = forward_chars = 3;
                }
                g_object_unref (pixbuf);
                gtk_text_buffer_delete_mark (self->buffer, mark);

                done = TRUE;
            }
            g_free (font);
        }
        if (forward_chars > 0)
        {
            gtk_text_iter_forward_chars (&iter, forward_chars);
        }
        if (tags)
        {
            g_slist_free (tags);
        }
    }
    while (gtk_text_iter_forward_to_tag_toggle (&iter, NULL));
}

#if MTX_TEXT_VIEW_DEBUG > 2
__attribute__((unused))
static void _print_slice (const int, const GtkTextIter *, const GtkTextIter);

static void
_print_slice (const int fd, const GtkTextIter *start, const GtkTextIter *end)
{
    gchar *slice = gtk_text_buffer_get_slice (self->buffer, start, end, TRUE);
    gint start_pos = gtk_text_iter_get_offset (start);
    gint end_pos = gtk_text_iter_get_offset (end);
    dprintf (fd, "[%d, %d](%s)\n", start_pos, end_pos, slice);
    g_free (slice);
}

#endif
/**
_get_string_pixel_size:
@string:
@tag: nullable
@width:
@height:
*/
static void
_get_string_pixel_size (MtxTextView *self,
                        gchar *string,
                        GtkTextTag *tag,
                        guint *width,
                        guint *height)
{
    PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (self));
    PangoFontDescription *desc = pango_context_get_font_description (context);
    PangoLayout *layout = pango_layout_new (context);
    pango_layout_set_text (layout, string, -1);
    pango_layout_set_font_description (layout, desc);
    if (tag)
    {
    /*
    Currently the returned width and height are only accurate if @tag doesn't
    override the default font properties that affect @layout size. (@tag derives
    from the markup, if any, that wrapped the markdown text leading to @string).
    To remove this limitation:
    TODO convert tag properties to pango markup and apply to layout.
    */;
    }
    pango_layout_get_pixel_size (layout, (int *) width, (int *) height);
    g_object_unref (layout);
}

/**
_indent_slice_lines:
@start:
@end:
@width: left-hanging indent in pixels of the first line at @start; after the
first line @width becomes the left margin in pixels for the remaining lines.
Note that if @end is not a line ending itself, then the indentation range
will extend through to the first line ending after @end.
*/
void
_indent_slice_lines (MtxTextView *self,
                     const GtkTextIter *start,
                     const GtkTextIter *end,
                     const guint width)
{
    GtkTextTag *indentt = NULL, *margint = NULL;
    GtkTextIter its = *start, ite = *end;
    gboolean multiline;
    GtkTextBuffer *buffer = gtk_text_iter_get_buffer (&its);

    if (width == 0)
    {
        gtk_text_buffer_apply_tag (buffer, self->indent_base_tag, &its, &ite);
        gtk_text_buffer_apply_tag (buffer, self->margin_base_tag, &its, &ite);
#if MTX_TEXT_VIEW_DEBUG > 2
        dprintf (2, "MARGIN % 4d px ", width);
        _print_slice (2, &its, &ite);
#endif
    }
    else
    {
        GtkTextIter line_end = its;
        multiline = gtk_text_iter_forward_to_line_end (&line_end)
            && gtk_text_iter_compare (&line_end, &ite) < 0;

#if MTX_TEXT_VIEW_DEBUG > 2
        dprintf (2, "INDENT % 4d px ", -width);
        _print_slice (2, &its, &line_end);
#endif
        /* hanging indent */
        indentt =
            gtk_text_buffer_create_tag (buffer, NULL, "indent", -width, NULL);
        gtk_text_buffer_apply_tag (buffer, indentt, &its, &line_end);

        if (multiline && gtk_text_iter_forward_line (&its))
        {
#if MTX_TEXT_VIEW_DEBUG > 2
            dprintf (2, "MARGIN % 4d px ", width);
            _print_slice (2, &its, &ite);
#endif
            margint =
            gtk_text_buffer_create_tag (buffer, NULL, "left-margin",
                                        width +
                                        MTX_TEXT_VIEW_LEFT_MARGIN, NULL);
            gtk_text_buffer_apply_tag (buffer, margint, &its, &ite);
        }
    }
}

/**
_get_rendered:
Return a copy of the string inside Pango @tag starting at @start.

@start: GtkTextIter pointer inside some text wrapped by *@tag.
@tag: pointer to GtkTextTag wrapping the @start position.

Return: pointer to newly-allocated memory.  The caller owns the returned
memory. Free it with _free_rendered.
*/
static MtxTextViewPrivateRendered *
_get_rendered (MtxTextView *self,
               const GtkTextIter *start,
               GtkTextTag *tag)
{
    GtkTextIter end = *start;
    GtkTextBuffer *buffer = gtk_text_iter_get_buffer (start);
    gtk_text_iter_forward_to_tag_toggle (&end, tag);
    MtxTextViewPrivateRendered *ret =
    g_malloc0 (sizeof (MtxTextViewPrivateRendered));
    ret->str =
    g_strdup (gtk_text_buffer_get_slice (buffer, start, &end, FALSE));
    ret->len =                         /* chars not bytes */
    gtk_text_iter_get_offset (&end) - gtk_text_iter_get_offset (start);
    _get_string_pixel_size (self, ret->str, tag, &ret->width, &ret->height);
    return ret;
}

/**
*/
static void
_free_rendered (MtxTextViewPrivateRendered *ptr)
{
    if (ptr)
    {
        g_free (ptr->str);
        g_free (ptr);
    }
}

/**
*/
static void
_text_buffer_set_invisible_to_eol (GtkTextBuffer *buffer,
                                   GtkTextIter *iter)
{

    /*
    At some point I was making the text invisible but then I switched to
    deleting the text for two reasons:
    1) This class should not add invisible text because it could overcomplicate
    the application implementation.
    2) I couldn't get invisible text to work due to
    https://stackoverflow.com/questions/59190178.
    */
    GtkTextIter end = *iter;
    gtk_text_iter_forward_to_line_end (&end);
    /* chomp also the line ending that trails each block quote level: I like a
    more compact view */
    gtk_text_iter_forward_line (&end);
    gtk_text_buffer_delete (buffer, iter, &end);
}

/**
_indent_blockquote:
Indent blockquote blocks. This should be the final indenting step.
*/
static void
_indent_blockquote (MtxTextView *self)
{
    MtxTextViewPrivateRendered *gap = NULL; /* between tab stops */
    GtkTextIter iter, prev_iter, gap_end;
    GtkTextTag *tag;
    GSList *tags, *tagp;
    gint lvl = -1, prev_lvl = 0;
    gboolean open = FALSE, prev_open = FALSE;
    guint block_cnt = 0;

    gtk_text_buffer_get_start_iter (self->buffer, &iter);
    prev_iter = iter;

    /* iter loop */
    do
    {
        tags = gtk_text_iter_get_tags (&iter);
        for (tagp = tags; tagp != NULL; tagp = tagp->next)
        {
            gchar *font = NULL;

            tag = tagp->data;
            g_object_get (G_OBJECT (tag), "font", &font, NULL);

            lvl = mtx_cmm_tag_get_info (self->markdown, font,
                                        MTX_TAG_BLOCKQUOTE_LEVEL);
            g_assert (lvl != 0);  /* lvl can be -1 or strictly > 0 */
            if (lvl > 0)
            {
                open = mtx_cmm_tag_get_info (self->markdown, font,
                                             MTX_TAG_BLOCKQUOTE_OPEN);

                /* Once: cache the formatted gap between blockquote_start&end */
                if (self->blockquote_start == NULL && open)
                {
                    MtxTextViewPrivateRendered *r =
                    _get_rendered (self, &iter, tag);
#if MTX_TEXT_VIEW_DEBUG > 1
                    /* Replace default symbol to see who does what. */
                    GString *s = g_string_new (r->str);
                    /* Don't change byte length! */
                    g_string_replace (s, "┌", "╠", 0);
                    g_free (r->str);
                    r->str = s->str;
                    g_string_free (s, FALSE);
#endif
                    self->blockquote_start = r;
                }
                if (self->blockquote_end == NULL && !open)
                    self->blockquote_end = _get_rendered (self, &iter, tag);

                /*
                Fill hanging indent with copies of cached gap string:
                prev_lvl repeated gap->str times = total hanging width.
                */
                if (prev_lvl > 0)
                {
                    GtkTextMark *prev_mark, *mark, *gap_end_mark;

                    gap =
                    prev_open ? self->blockquote_start : self->blockquote_end;

                    /* The marks dance. */
                    prev_mark = gtk_text_buffer_create_mark (self->buffer, NULL,
                                                             &prev_iter, TRUE);
                    mark = gtk_text_buffer_create_mark (self->buffer, NULL,
                                                        &iter, FALSE);
                    gap_end = prev_iter;
                    gtk_text_iter_forward_chars (&gap_end, gap->len);
                    gap_end_mark =
                    gtk_text_buffer_create_mark (self->buffer, NULL, &gap_end,
                                                 FALSE);

                    /* Fill. */
                    for (gint j = prev_lvl; j > 1; j--)
                    {
                        gtk_text_buffer_insert (self->buffer, &prev_iter,
                                                gap->str, -1);
                    }

                    /*
                    Apply formatting to the hanging indentation part.
                    */
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &prev_iter,
                                                      prev_mark);
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &gap_end,
                                                      gap_end_mark);
                    gtk_text_buffer_apply_tag (self->buffer, tag, &prev_iter,
                                               &gap_end);
                    if (!prev_open)
                    {
                        /* Hide closing symbols to avoid visual noise.  */
                        _text_buffer_set_invisible_to_eol (self->buffer,
                                                           &prev_iter);
                    }
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &prev_iter,
                                                      prev_mark);
                    gtk_text_buffer_get_iter_at_mark (self->buffer, &iter, mark);
                }

                guint width = gap && prev_open > 0 ? prev_lvl * gap->width : 0;
                _indent_slice_lines (self, &prev_iter, &iter, width);

                prev_lvl = lvl;
                prev_open = open;
                prev_iter = iter;
                block_cnt++;
            }
            g_free (font);
            break;
        }
        if (tags)
        {
            g_slist_free (tags);
        }
    }
    while (gtk_text_iter_forward_to_tag_toggle (&iter, NULL));

    if (block_cnt)
    {
        gtk_text_buffer_get_end_iter (self->buffer, &iter);
        _indent_slice_lines (self, &prev_iter, &iter, 0);
        /* Hide closing symbol to avoid visual noise. */
        _text_buffer_set_invisible_to_eol (self->buffer, &prev_iter);
    }
}

/**
_indent_li:
Indent list elements.
*/
static void
_indent_li (MtxTextView * self)
{
    GtkTextIter iter;
    GtkTextTag *tag;
    GSList *tags, *tagp;
    gint li_lvl = -1;
    gint ol_ul_lvl;
    gint this_li_id, maybe_li_id;
    gboolean cont;
    GString *fill = g_string_new ("                ");  /* 16 spaces */

    gtk_text_buffer_get_start_iter (self->buffer, &iter);

    /* iter loop */
    do
    {
        tags = gtk_text_iter_get_tags (&iter);
        for (tagp = tags; tagp != NULL; tagp = tagp->next)
        {
            g_autofree gchar *font = NULL;
            g_free (font);

            tag = tagp->data;
            g_object_get (G_OBJECT (tag), "font", &font, NULL);
#ifdef MTX_DEBUG
            {
                gint s, w;
                g_object_get (G_OBJECT (tag), "style", &s, NULL);
                g_object_get (G_OBJECT (tag), "weight", &w, NULL);
                mtx_dbg_errout (-1, "style %d weight %d font %s\n", s, w, font);
            }
#endif

            li_lvl =
            mtx_cmm_tag_get_info (self->markdown, font, MTX_TAG_LI_LEVEL);
            g_assert (li_lvl); /* -1 || > 0 for a valid LI element */
            if (li_lvl > 0)
            {
                gchar *bullet;
                guint bullet_len, margin, hang, height;
                GtkTextIter end = iter;
                GtkTextMark *mrk, *mark_end;
                /*
                Calculate and apply indentation depth and hanging indent width.
                GtkTextBuffer sets margin and indent relative to x-coordinate
                zero.  We round the starting position of the left-hanging
                indent to the nearest multiple of ASCII space width in pixels
                (indent_quantum).
                */

                /* Once: cache ASCII space width. */
                if (self->indent_quantum <= 0)
                {
                    _get_string_pixel_size (self, " ", NULL,
                                              &(self->indent_quantum), &height);
                    g_assert (self->indent_quantum > 0);
                }

#if 0
                gint ordinal =
                mtx_cmm_tag_get_info (self->markdown, font, MTX_TAG_LI_ORDINAL);
#endif
                bullet_len = mtx_cmm_tag_get_info (self->markdown, font,
                                                   MTX_TAG_LI_BULLET_LEN);

                /* width = 60 + 20 * li_lvl; */
                gtk_text_iter_forward_chars (&end, bullet_len);
                bullet =
                gtk_text_buffer_get_slice (self->buffer, &iter, &end, TRUE);
                _get_string_pixel_size (self, bullet, NULL, &hang, &height);
                g_free (bullet);

                margin = 20 + li_lvl * 20;
                g_assert (hang <= margin);
                if (hang < margin)
                {
                    /* How many spaces to fill the hanging indent? */
                    guint n = (margin - hang) / self->indent_quantum;
                    guint m0 = n * self->indent_quantum + hang;
                    guint m1 = (n + 1) * self->indent_quantum + hang;
                    if (ABS ((gint) (margin - m0)) > ABS ((gint) (margin - m1)))
                    {
                        ++n;
                    }
                    /* Fill. */
                    if (n > 0)
                    {
                        if (n >= fill->len && n < 2048)
                        {
                            g_string_append (fill, fill->str);
                        }
                        mrk = gtk_text_buffer_create_mark (self->buffer, NULL,
                                                           &iter, TRUE);
                        mark_end =
                        gtk_text_buffer_create_mark (self->buffer, NULL, &end,
                                                     FALSE);
                        gtk_text_buffer_insert (self->buffer, &iter, fill->str,
                                                n % fill->len);
                        gtk_text_buffer_get_iter_at_mark (self->buffer, &iter,
                                                          mrk);
                        gtk_text_buffer_get_iter_at_mark (self->buffer, &end,
                                                          mark_end);
                    }
                }
                else
                    /* Shouldn't happen. */
                {
                    hang = margin;
                }


                /*
                Find this LI's start segment. Forward from LI's start to either
                LI's end or the next LI or an open/close OL/UL, whichever
                comes first.  This LI's body is preceded by (assuming _ID==N)
                <span font="...liId=N"></span> and followed by
                <span font="...liId=N">sUNIPUA_PANGO_EMPTY_SPAN</span>.
                */
                this_li_id =
                mtx_cmm_tag_get_info (self->markdown, font, MTX_TAG_LI_ID);
                cont = TRUE;
                do
                {
                    GSList *ts, *tp;
                    GtkTextTag *ta;
                    gchar *prop = NULL;
                    ol_ul_lvl = -1;

                    gtk_text_iter_forward_to_tag_toggle (&end, NULL);
                    ts = gtk_text_iter_get_tags (&end);
                    for (tp = ts; tp != NULL && cont; tp = tp->next)
                    {
                        ta = tp->data;
                        g_object_get (G_OBJECT (ta), "font", &prop, NULL);
                        if (prop && *prop)
                        {
                            maybe_li_id = mtx_cmm_tag_get_info (self->markdown,
                                                                prop,
                                                                MTX_TAG_LI_ID);
                            mtx_dbg_errout (-1, "this(%d) maybe(%d) prop(%s)\n",
                                            this_li_id, maybe_li_id, prop);
                            if (maybe_li_id == this_li_id || maybe_li_id > 0 ||
                                (ol_ul_lvl = mtx_cmm_tag_get_info
                                 (self->markdown, prop,
                                  MTX_TAG_OL_UL_LEVEL)) > 0)
                            {
                                cont = FALSE;
                            }
                        }
                        g_free (prop);
                    }
                    if (ts)
                    {
                        g_slist_free (ts);
                    }
                }
                while (cont && ol_ul_lvl != 0);
                mtx_dbg_errout (-1, "this(%d) ==? maybe(%d)\n", this_li_id,
                                maybe_li_id);

                _indent_slice_lines (self, &iter, &end, margin);
                iter = end;
                if (maybe_li_id > 0 && maybe_li_id != this_li_id)
                {
                    gtk_text_iter_backward_to_tag_toggle (&iter, NULL);
                }
            }
            break;
        }
        if (tags)
        {
            g_slist_free (tags);
        }
    }
    while (gtk_text_iter_forward_to_tag_toggle (&iter, NULL));
    g_string_free (fill, TRUE);
}

/**
_text_buffer_delete_unichar_all:
Delete all occurrences of a Unicode code point.
*/
static void
_text_buffer_delete_unichar_all (MtxTextView *self,
                                 gunichar code_point)
{
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter (self->buffer, &iter);

    while (!gtk_text_iter_is_end (&iter))
    {
        gunichar ch = gtk_text_iter_get_char (&iter);
        if (ch == code_point)
        {
            GtkTextIter next = iter;
            gtk_text_iter_forward_char (&next);
            gtk_text_buffer_delete (self->buffer, &iter, &next);
            iter = next;
        }
        else
        {
            gtk_text_iter_forward_char (&iter);
        }
    }
}

static void
_indent_text_buffer (MtxTextView *self)
{
    _indent_li (self);
    _indent_blockquote (self);
    _text_buffer_delete_unichar_all (self, iUNIPUA_PANGO_EMPTY_SPAN);
}

/**
mtx_text_view_set_text:
The markdown-to-text-view main entry point:
Convert markdown text to Pango markup; resolve images; insert results into the
text view buffer; re-prioritize buffer text tags, and apply indentation.

@self:
@text: address of a pointer to the markdown text string.
@referrer: the base pathname context for loading images. See also
#mtx_text_view_load_file.
@clear_text: if TRUE, free the memory pointed to by @text and set @text to NULL.

Returns: FALSE on parsing error otherwise return TRUE.
*/
gboolean
mtx_text_view_set_text (MtxTextView *self,
                        gchar **text,
                        const gchar *referrer,
                        const gboolean clear_text)
{
    gchar *markup = NULL;
    gboolean result = TRUE;

    g_return_val_if_fail (IS_MTX_TEXT_VIEW (self), FALSE);

    mtx_text_view_reset (self);
#ifdef MTX_TEXT_VIEW_DEBUG
    if (gl_text_view_debug_markup_file != NULL)
    {
        gsize sz;

        if (clear_text)
        {
            g_free (*text); /* superseded by the debug file to be loaded */
            *text = NULL;
        }
        markup =
        _get_file_contents (gl_text_view_debug_markup_file, &sz, TRUE);
        g_printerr ("%s: read %lu bytes from %s\n", __FUNCTION__, sz,
                    gl_text_view_debug_markup_file);
    }
    else
#endif
    {
        markup = mtx_cmm_mtx (self->markdown, text, NULL, clear_text);
    }
    if (markup != NULL)
    {
        GtkTextIter iter;

        gtk_text_buffer_set_text (self->buffer, "\n", 1);
        gtk_text_buffer_get_start_iter (self->buffer, &iter);
        _text_buffer_insert_markup (self->buffer, &iter, markup);
        g_free (markup);

        load_images_and_mark_links (self, referrer);

       /* set highlight tag's priority above text tags inserted from markup */
        guint tsz =
        gtk_text_tag_table_get_size (gtk_text_buffer_get_tag_table
                                     (self->buffer));
        gtk_text_tag_set_priority (self->highlight_tag, tsz - 1);

        _indent_text_buffer (self);
    }
    return result;
}

/**
mtx_text_view_load_file:
Load a markdown file into the text view.

@self:
@file: the markdown file.
@referrer: pathname (not necessarily directory, not necessarily absolute)
context for resolving relative image and link paths.  Can be "" but not NULL.
@utf8_validate: if TRUE validate the UTF-8 encoding of file data.

Returns: TRUE and emits signal "file-load-complete" if the file was loaded and
the text view filled, otherwise it returns FALSE.
*/
gboolean
mtx_text_view_load_file (MtxTextView *self,
                         const gchar *file,
                         const gchar *referrer,
                         const gboolean utf8_validate)
{
    g_return_val_if_fail (IS_MTX_TEXT_VIEW (self), FALSE);
    g_return_val_if_fail (file && file[0] && referrer, FALSE);
    g_return_val_if_fail (self->image_directory, FALSE);

    g_autofree gchar *basedir = NULL;
    g_autofree gchar *abs_img_dir = NULL;
    g_autofree gchar *path = NULL;
    g_autofree gchar *contents = NULL;
    gboolean retval = FALSE;
    gboolean is_abs_referrer = g_path_is_absolute (referrer);

    if (g_path_is_absolute (file))
    {
        contents =
        mtx_text_view_get_file_contents (self, file, NULL, utf8_validate);
    }
    else
    {
        abs_img_dir = g_canonicalize_filename (self->image_directory, NULL);
        path = g_build_filename (abs_img_dir, file, NULL);
        contents =
        mtx_text_view_get_file_contents (self, path, NULL, utf8_validate);
    }

    if (contents == NULL)
    {
        /* retry relative to referrer's directory */

        g_free (path);

        if (is_abs_referrer)
        {
            basedir = g_path_get_dirname (referrer);
            path = g_build_filename (basedir, file, NULL);
        }
        else
        {
            if (abs_img_dir == NULL)
            {
                abs_img_dir =
                g_canonicalize_filename (self->image_directory, NULL);
            }
            gchar *q = g_canonicalize_filename (referrer, abs_img_dir);
            basedir = g_path_get_dirname (q);
            path = g_build_filename (basedir, file, NULL);
            g_free (q);
        }
        contents =
        mtx_text_view_get_file_contents (self, path, NULL, utf8_validate);
    }
    if (contents != NULL)
    {
        retval = mtx_text_view_set_text (self, &contents, path, TRUE);
        if (retval)
        {
            g_signal_emit (self, mtx_text_view_signals[FILE_LOAD_COMPLETE],
                           0, file);
        }
    }
    return retval;
}

void
mtx_text_view_set_image_directory (MtxTextView *self,
                                   const gchar *directory)
{
    g_return_if_fail (IS_MTX_TEXT_VIEW (self));

    g_free (self->image_directory);
    self->image_directory = g_strdup (directory);
}

void
mtx_text_view_set_auto_lang_find (MtxTextView *self,
                                  const gboolean enable)
{
    g_return_if_fail (IS_MTX_TEXT_VIEW (self));
    static gchar **languages = NULL;

    if ((enable && self->auto_languages != NULL)
        || (!enable && self->auto_languages == NULL))
    {
        return;
    }
    else if (!enable)
    {
        self->auto_languages = NULL;
    }
    else if (enable && languages != NULL)
    {
        self->auto_languages = languages;
    }
    else
    {
        gchar **strv = (gchar **) g_get_language_names (); /* ends with ":C" */
        if (g_strv_length (strv) > 1)
        {
            gchar *p, *q, *lngs;
            g_autofree gchar *temp = g_strjoinv (":", strv);

            /* left trim ':' and squeeze interior runs of ':' */
            for (p = temp; *p && *p == ':'; p++)
                ;
            for (lngs = q = p, p = p + 1; *p; p++)
            {
                if (*p != ':' || *q != ':')
                {
                    *++q = *p;
                }
            }
            /* chop ":C" */
            *q = '\0';
            if (q[-1] == ':')
            {
                q[-1] = '\0';
            }

            languages = g_strsplit (lngs, ":", -1);
            self->auto_languages = languages;
        }
    }
    return;
}

void
mtx_text_view_set_extensions (MtxTextView *self,
                              const MtxCmmExtensions flags)
{
    g_return_if_fail (IS_MTX_TEXT_VIEW (self));

    mtx_cmm_set_extensions (self->markdown, flags);
    return;
}

void
mtx_text_view_set_tweaks (MtxTextView *self,
                          const MtxCmmTweaks flags)
{
    g_return_if_fail (IS_MTX_TEXT_VIEW (self));

    mtx_cmm_set_tweaks (self->markdown, flags);
    return;
}

/**
mtx_text_view_clear_search_highlights:
Clear the highlights left behind when `mtx_text_view_find_text` search
options does not include #MTX_TEXT_VIEW_SEARCH_ONE_HILIGHT.
*/
void
mtx_text_view_clear_search_highlights (MtxTextView *self)
{
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (self->buffer, &start, &end);
    gtk_text_buffer_remove_tag (self->buffer, self->highlight_tag,
                                &start, &end);
}

/**
mtx_text_view_find_text:
Search the text buffer for the next occurrence of term.
Matches are not case sensitive.

@search_text:
@options: %MtxTextViewSearchOptions bit mask.
*/
gboolean
mtx_text_view_find_text (MtxTextView *self,
                         const gchar *search_text,
                         MtxTextViewSearchOptions options)
{
    GtkTextIter start_iter, match_start, match_end;
    GtkTextIter doc_start_iter, doc_end_iter;
    GtkTextMark *mark;
    gboolean found;
#if GTK_CHECK_VERSION (3,0,0)
    const GtkTextSearchFlags flags =
    GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE;
#else
    const GtkTextSearchFlags flags = GTK_TEXT_SEARCH_TEXT_ONLY;
#endif
    gchar *needle = g_shell_unquote (search_text, NULL);

    mark = gtk_text_buffer_get_insert (self->buffer);
    gtk_text_buffer_get_bounds (self->buffer, &doc_start_iter, &doc_end_iter);

    if (options & MTX_TEXT_VIEW_SEARCH_ONE_HILIGHT)
    {
        mtx_text_view_clear_search_highlights (self);
    }
    if (!(options & MTX_TEXT_VIEW_SEARCH_BACK))
    {
        /* search forward */
        gtk_text_buffer_get_start_iter (self->buffer, &start_iter);
        if (mark)
        {
            gtk_text_buffer_get_iter_at_mark (self->buffer, &start_iter, mark);
            /* start searching from cursor + 1 */
            if (!gtk_text_iter_forward_cursor_position (&start_iter))
            {
                /* can't move forward, at end of page, start at top */
                gtk_text_buffer_get_start_iter (self->buffer, &start_iter);
            }
        }
        /* first attempt */
        found =
        gtk_text_iter_forward_search (&start_iter, needle, flags,
                                      &match_start, &match_end, NULL);
        /* wrap around */
        if (!found && gtk_text_iter_compare (&start_iter, &doc_start_iter))
        {
            found =
            gtk_text_iter_forward_search (&doc_start_iter, needle, flags,
                                          &match_start, &match_end, NULL);
        }
    }
    else
    {
        /* search backward */
        gtk_text_buffer_get_end_iter (self->buffer, &start_iter);
        if (mark)
        {
            gtk_text_buffer_get_iter_at_mark (self->buffer, &start_iter, mark);
            /* start searching from cursor */
            if (gtk_text_iter_is_start (&start_iter))
            {
                /* at top of page, start at bottom */
                gtk_text_buffer_get_end_iter (self->buffer, &start_iter);
            }
        }
        /* first attempt */
        found =
        gtk_text_iter_backward_search (&start_iter, needle, flags,
                                       &match_start, &match_end, NULL);
        /* wrap around */
        if (!found && gtk_text_iter_compare (&start_iter, &doc_end_iter))
        {
            found =
            gtk_text_iter_backward_search (&doc_end_iter, needle, flags,
                                           &match_start, &match_end, NULL);
        }
    }
    g_free (needle);
    if (found)
    {
        gtk_text_buffer_place_cursor (self->buffer, &match_start);
        gtk_text_buffer_apply_tag (self->buffer, self->highlight_tag,
                                   &match_start, &match_end);
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (self), mark);
    }
    return found;
}

/**
*/
static gboolean
link_dest_id_equal (gconstpointer *a,
                    gconstpointer *b)
{
    return ((MtxTextViewLinkInfo *) a)->link_dest_id ==
           ((MtxTextViewLinkInfo *) b)->link_dest_id;
}

/**
mtx_text_view_highlight_at_cursor:
Highlight the link text span that contains the cursor position.  If the cursor
isn't contained in a link text span then highlight the word the cursor is in.

@self: pointer to #MtxTextView.
@mode: Highlight or select the span.
*/
void
mtx_text_view_highlight_at_cursor (MtxTextView *self,
                                   const MtxTextViewHilightMode mode)
{
    GtkTextIter start, end;
    guint index;
    gint link_dest_id;

    GtkTextMark *mark_at_cursor = gtk_text_buffer_get_insert (self->buffer);
    gtk_text_buffer_get_iter_at_mark (self->buffer, &start, mark_at_cursor);
    end = start;

    link_dest_id =
    mtx_text_view_get_link_at_iter (self, &start, NULL, NULL);
    if (link_dest_id >= 0)
    {
        MtxTextViewLinkInfo info = {NULL, link_dest_id, 0};
        if (g_ptr_array_find_with_equal_func (self->link_marks, &info,
                                              (GEqualFunc) link_dest_id_equal,
                                              &index))
        {
            /* Expand highlighting over the whole link text span. */
            guint offset;
            GtkTextIter lstart;
            MtxTextViewLinkInfo *lmark =
            g_ptr_array_index (self->link_marks, index);

            gtk_text_buffer_get_iter_at_mark (self->buffer, &lstart,
                                              lmark->mark);
            offset =
            gtk_text_iter_get_offset (&start) -
            gtk_text_iter_get_offset (&lstart);
            mtx_dbg_errout (-1,
                            "cursor inside link span (length %d, offset %d)",
                            lmark->llen, offset);
            if (offset > 0)
            {
                gtk_text_iter_backward_chars (&start, offset);
            }
            end = start;
            gtk_text_iter_forward_chars (&end, lmark->llen);
        }
    }
    else
    {
        if (!gtk_text_iter_starts_word (&start))
        {
            if (gtk_text_iter_backward_word_start (&start))
            {
                mtx_dbg_errseq (-1, "%s", " | cursor moved to word start");
            }
        }
        if (!gtk_text_iter_ends_word (&end))
        {
            if (gtk_text_iter_forward_word_end (&end))
            {
                mtx_dbg_errseq (-1, "%s"," | cursor moved to word end");
            }
        }
    }
    if (mode == MTX_TEXT_VIEW_HILIGHT_NORMAL)
    {
        gtk_text_buffer_apply_tag (self->buffer, self->highlight_tag,
                                   &start, &end);
    }
    else if (mode & MTX_TEXT_VIEW_HILIGHT_SELECT)
    {
        gtk_text_buffer_select_range (self->buffer, &start, &end);
    }
}

/**
mtx_text_view_highlight_at_cursor_chars:
Highlight a text span starting at the cursor position.

@self: pointer to #MtxTextView.
@chars: how many characters to highlight from the cursor position included
onward.
@mode: Highlight or select the span.
*/
void
mtx_text_view_highlight_at_cursor_chars (MtxTextView *self,
                                         const guint chars,
                                         const MtxTextViewHilightMode mode)
{
    GtkTextIter start, end;

    _text_buffer_get_cursor (self->buffer, &start);
    end = start;
    gtk_text_iter_forward_chars (&end, chars);
    if (mode == MTX_TEXT_VIEW_HILIGHT_NORMAL)
    {
        gtk_text_buffer_apply_tag (self->buffer, self->highlight_tag, &start, &end);
    }
    else if (mode & MTX_TEXT_VIEW_HILIGHT_SELECT)
    {
        gtk_text_buffer_select_range (self->buffer, &start, &end);
    }
}

void
mtx_text_view_cursor_to_top (MtxTextView *self)
{
    GtkTextIter start_iter;

    gtk_text_buffer_get_start_iter (self->buffer, &start_iter);
    gtk_text_buffer_place_cursor (self->buffer, &start_iter);
}

static void
mtx_text_view_init (MtxTextView *self)
{
    self->markdown = mtx_cmm_new ();
    self->image_directory = g_strdup (".");

    mtx_cmm_set_output (self->markdown, MTX_CMM_OUTPUT_PANGO);
    mtx_cmm_set_escape (self->markdown, TRUE);

    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self), GTK_WRAP_WORD);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (self), FALSE);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (self),
                                   MTX_TEXT_VIEW_LEFT_MARGIN);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (self),
                                    MTX_TEXT_VIEW_RIGHT_MARGIN);
    gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (self),
                                          MTX_TEXT_VIEW_PIXEL_ABOVE_LINE);
    gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (self),
                                          MTX_TEXT_VIEW_PIXEL_BELOW_LINE);

    self->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
    self->margin_base_tag =
    gtk_text_buffer_create_tag (self->buffer, "margin-base", "left-margin",
                                MTX_TEXT_VIEW_LEFT_MARGIN, NULL);
    self->indent_base_tag =
    gtk_text_buffer_create_tag (self->buffer, "indent-base", "indent", 0, NULL);
    self->highlight_tag =
    gtk_text_buffer_create_tag (self->buffer, "highlight",
                                "background", MTX_COLOR_HIGHLIGHT_BG,
                                /*"foreground", MTX_COLOR_HIGHLIGHT_FG, */
                                NULL);
    self->link_marks = g_ptr_array_new ();
    self->auto_languages = NULL;

    g_signal_connect (self, "event-after", G_CALLBACK (event_after), NULL);
    g_signal_connect (self, "key-press-event",
                      G_CALLBACK (key_press_event), NULL);
    g_signal_connect (self, "motion-notify-event",
                      G_CALLBACK (motion_notify_event), NULL);
    g_signal_connect (self, "visibility-notify-event",
                      G_CALLBACK (visibility_notify_event), NULL);

    {
#if !GTK_CHECK_VERSION(3,0,0)
        GdkColor clr;
        if (gdk_color_parse (MTX_COLOR_FG, &clr))
        {
            gtk_widget_modify_text (GTK_WIDGET (self), GTK_STATE_NORMAL, &clr);
        }
        if (gdk_color_parse (MTX_COLOR_BG, &clr))
        {
            gtk_widget_modify_base (GTK_WIDGET (self), GTK_STATE_NORMAL, &clr);
        }
#else
        GtkCssProvider *provider;
        GtkStyleContext *context;
        gchar *css = g_strconcat (".view, .view text {",
                                  "color: " MTX_COLOR_FG ";",
                                  "background-color: " MTX_COLOR_BG ";",
                                  "}", NULL);
        provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data (provider, css, -1, NULL);
        context = gtk_widget_get_style_context (GTK_WIDGET (self));
        gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER
                                        (provider),
                                        GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_free (css);
#endif
    }
}

/**
_get_file_contents:
Memory-mapped file read utility function.

@path: file to read.
@size: pointer to the size of the returned string. NULLABLE.
@utf8_validate: if TRUE validate the UTF-8 encoding of file data.

Returns: a newly-allocated string holding the contents of the file, and sets
*size to the string size in bytes (without counting the terminating '\0' byte.
On error it returns NULL, *size is undefined, and errno is set to the error
number.
*/
gchar *
_get_file_contents (const gchar *path,
                    gsize *size,
                    const gboolean utf8_validate)
{
    int fd;
    gchar *buffer = NULL;
    const gchar *invalid;
    gsize sz = 0;
    if ((fd = open (path, O_RDONLY)) >= 0)
    {
        struct stat sb;
        fstat (fd, &sb);
        gchar *mapped =
        mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED | MAP_NORESERVE, fd, 0);
        close (fd);
        if (mapped != MAP_FAILED)
        {
            buffer = g_malloc (sb.st_size + 1);
            memcpy (buffer, mapped, sb.st_size);
            munmap (mapped, sb.st_size);
            buffer[sz = sb.st_size] = 0;
            if (utf8_validate && !g_utf8_validate (buffer, -1, &invalid))
            {
                g_critical ("%s: invalid UTF-8 data at offset %ld", path,
                            invalid - buffer);
                g_free (buffer);
                buffer = NULL;
            }
        }
        else if (sb.st_size == 0)
        {
            buffer = g_malloc (1);
            *buffer = '\0';
        }
    }
    if (size != NULL)
    {
        *size = sz;
    }
    return buffer;
}

/**
mtx_text_view_auto_lang_find:
Find the closest language-specific replacement file.

@path: File[.ext]

Returns: a newly-allocated string holding the closest replacement path if one
exists, otherwise it returns NULL.

The first existing replacement of File.ext from the list File.<list-lookup>.ext.
The replacement is looked for in the <list-lookup> returned by
https://libsoup.org/glib/glib-I18N.html#g-get-language-names.
*/
gchar *
mtx_text_view_auto_lang_find (MtxTextView *self,
                              const gchar *path)
{
    g_return_val_if_fail (IS_MTX_TEXT_VIEW (self), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    if (self->auto_languages == NULL)
    {
        return NULL;
    }
    g_return_val_if_fail (path && *path, NULL);
    gchar *result, **lang;
    g_autofree const gchar *dirname = g_path_get_dirname (path);
    g_autofree gchar *basename = g_path_get_basename (path);
    gchar *ext = NULL;
    gchar *dot = strrchr (basename, '.');
    if (dot != NULL)
    {
        *dot = '\0';
        ext = dot + 1;
    }
    for (lang = self->auto_languages; *lang; lang++)
    {
        gchar *filename;
        if (ext == NULL)
        {
           filename = g_strconcat (basename, ".", lang, NULL);
        }
        else
        {
           filename = g_strconcat (basename, ".", *lang, ".", ext, NULL);
        }
        mtx_dbg_errout (-1, "%s", "");
        result = g_build_filename (dirname, filename, NULL);
        g_free (filename);
        mtx_dbg_errseq (-1, "exists? (%s)", result);
        if (g_file_test (result, G_FILE_TEST_EXISTS))
        {
            mtx_dbg_errseq (-1, "%s FOUND!\n", "");
            return result;
        }
    }
    mtx_dbg_errseq (-1, "%s\n", "");
    return NULL;
}

/**
mtx_text_view_get_file_contents:
Load a markdown file looking for a language-specific replacement with
#mtx_text_view_auto_lang_find.

@self:
@path: the file to load.
@size: pointer to file size return value. NULLABLE.
@utf8_validate: if TRUE validate the UTF-8 encoding of file data.

Returns: a newly-allocated string holding the contents of the file, and sets
*size to the string size in bytes.  On error, it returns NULL, *size is
undefined, and errno is set to the error number.
*/
gchar *
mtx_text_view_get_file_contents (MtxTextView *self,
                                 const gchar *path,
                                 gsize *size,
                                 const gboolean utf8_validate)
{
    g_return_val_if_fail (IS_MTX_TEXT_VIEW (self), NULL);
    g_autofree const gchar *altpath =
    mtx_text_view_auto_lang_find (self, path);
    return _get_file_contents (altpath ? altpath : path, size, utf8_validate);
}

static void
mtx_text_view_dispose (GObject *gobject)
{
    MtxTextView *priv =
    mtx_text_view_get_instance_private (MTX_TEXT_VIEW (gobject));

    g_clear_object (&priv->markdown);  /* NOLINT(bugprone-sizeof-expression) */

    G_OBJECT_CLASS (mtx_text_view_parent_class)->dispose (gobject);
}

static void
mtx_text_view_finalize (GObject *gobject)
{
    MtxTextView *priv =
    mtx_text_view_get_instance_private (MTX_TEXT_VIEW (gobject));

    if (priv->markdown)
    {
        g_object_unref (priv->markdown);
    }
    _free_rendered (priv->blockquote_start);
    _free_rendered (priv->blockquote_end);
    for (guint i = 0; i < priv->link_marks->len; i++)
    {
        g_free (g_ptr_array_index (priv->link_marks, i));
    }
    g_ptr_array_free (priv->link_marks, TRUE);
    g_strfreev (priv->auto_languages);
    g_free (priv->image_directory);

    G_OBJECT_CLASS (mtx_text_view_parent_class)->finalize (gobject);
}

