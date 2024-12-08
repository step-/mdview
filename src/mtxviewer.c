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
This file was derived from the mdview3 help-viewer.c with substantial changes
and feature additions for MTX. The mdview3 help-viewer.c itself was derived
from the hardinfo "help-viewer" directory.
*/
/*
 *    HelpViewer - Simple Help file browser
 *    Copyright (C) 2009 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2015 James B
 *    Copyright (C) 2023 step
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#define _GNU_SOURCE                      /* for strcasestr() */
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_66
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <glib/gi18n.h> /* xgettext --keyword=_ --keyword=Q_:1g */
#include <locale.h>

#include "mtxcmm.h"
#include "mtxtextview.h"
#include "mtxviewer.h"
#include "mtxversion.h"
#include "mtxdbg.h"

#define STATUSBAR_CTX_MAIN 0
#define STATUSBAR_CTX_LINK 1
#define STATUSBAR_CTX_WARN 2

typedef struct mtx_viewer_nav_unit
{
    gchar *file;
    guint offset;
} MtxViewerNavUnit;

static gboolean do_file_search (MtxViewer *, const gchar *);
static gboolean do_resource_load (MtxViewer *, const gchar *, const gchar *);
static void file_load_complete (MtxTextView *, const gchar *, gpointer);
static void on_curpos_changed (GtkTextBuffer *, GParamSpec *, gpointer);

#ifdef VIEWER_DEBUG

static void
_nav_trail_print (MtxViewer *mvr)
{
    MtxViewerNavUnit *unit;
    gint nav_trail_length;
    MtxViewerNavUnit *page;

    if (mvr->nav_trail_page == NULL)
    {
        return;
    }
    page = (MtxViewerNavUnit *) mvr->nav_trail_page;
    g_printerr ("#back(%d)", mvr->nav_trail_page_idx);
    for (gint i = 0; i < mvr->nav_trail_page_idx; i++)
    {
        unit = g_queue_peek_nth (mvr->nav_trail, i);
        g_printerr(" « (%d)(%s)", unit->offset, unit->file);
    }
    g_printerr (" « \033[7m (%d)(%s) \033[0m »", page->offset, page->file);
    nav_trail_length = g_queue_get_length (mvr->nav_trail);
    for (gint i = mvr->nav_trail_page_idx + 1; i < nav_trail_length; i++)
    {
        unit = g_queue_peek_nth (mvr->nav_trail, i);
        g_printerr(" (%d)(%s) »", unit->offset, unit->file);
    }
    g_printerr (" #fore(%d) changed_curpos(%d)\n", nav_trail_length - mvr->nav_trail_page_idx - 1, mvr->changed_curpos);
}
#endif /* VIEWER_DEBUG */

/**
_nav_trail_print_status_bar:
*/
static void
_nav_trail_print_status_bar (MtxViewer *mvr)
{
    MtxViewerNavUnit *unit;
    gint nav_trail_length;
    MtxViewerNavUnit *page;
    gchar *name;
    GString *message = g_string_new (NULL);

    if (mvr->nav_trail_page == NULL)
    {
        return;
    }
    page = (MtxViewerNavUnit *) mvr->nav_trail_page;
    for (gint i = 0; i < mvr->nav_trail_page_idx; i++)
    {
        unit = g_queue_peek_nth (mvr->nav_trail, i);
        name = g_path_get_basename (unit->file);
        g_string_append_printf (message, " « %s", name);
        g_free (name);
    }
    if (mvr->nav_trail_page_idx > 0)
    {
        g_string_append (message, " \u25C0");
    }
    name = g_path_get_basename (page->file);
    g_string_append_printf (message, " %s ", name);
    g_free (name);
    nav_trail_length = g_queue_get_length (mvr->nav_trail);
    if (nav_trail_length - mvr->nav_trail_page_idx - 1 > 0)
    {
        g_string_append (message, "\u25B6 ");
    }
    for (gint i = mvr->nav_trail_page_idx + 1; i < nav_trail_length; i++)
    {
        unit = g_queue_peek_nth (mvr->nav_trail, i);
        name = g_path_get_basename (unit->file);
        g_string_append_printf (message, " %s »", name);
        g_free (name);
    }
    gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK);
    gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_WARN);
    gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_MAIN,
                        message->str);
    g_string_free (message, TRUE);
}

/**
_nav_trail_back:
Go back one step on the navigation trail, and disable the back button if the
trail start is reached.

@mvr: pointer to #MtxViewer instance
*/
static void
_nav_trail_back (MtxViewer *mvr)
{
    mvr->nav_trail_page_idx--;
    mvr->nav_trail_page =
    g_queue_peek_nth (mvr->nav_trail, mvr->nav_trail_page_idx);
    if (mvr->nav_trail_page_idx == 0)
    {
        gtk_widget_set_sensitive (mvr->btn_nav_back, mvr->can_go_back = FALSE);
    }
    if (!mvr->can_go_fore)
    {
        gtk_widget_set_sensitive (mvr->btn_nav_fore, mvr->can_go_fore = TRUE);
    }
    _nav_trail_print_status_bar (mvr);
#ifdef VIEWER_DEBUG
        g_printerr ("+++++++++++ %s: ", __FUNCTION__); _nav_trail_print (mvr);
#endif // VIEWER_DEBUG
}

/**
_nav_trail_fore:
Go forward one step on the navigation trail, and disable the fore button if the
trail end is reached.

@mvr: pointer to #MtxViewer instance
*/
static void
_nav_trail_fore (MtxViewer *mvr)
{
    mvr->nav_trail_page_idx++;
    mvr->nav_trail_page =
    g_queue_peek_nth (mvr->nav_trail, mvr->nav_trail_page_idx);
    if (mvr->can_go_fore && mvr->nav_trail_page_idx == (gint)
        (g_queue_get_length (mvr->nav_trail) - 1))
    {
        gtk_widget_set_sensitive (mvr->btn_nav_fore, mvr->can_go_fore = FALSE);
    }
    if (!mvr->can_go_back && mvr->nav_trail_page_idx > 0)
    {
        gtk_widget_set_sensitive (mvr->btn_nav_back, mvr->can_go_back = TRUE);
    }
    _nav_trail_print_status_bar (mvr);
#ifdef VIEWER_DEBUG
    g_printerr ("+++++++++++ %s: ", __FUNCTION__); _nav_trail_print (mvr);
#endif // VIEWER_DEBUG
}

/**
_nav_trail_insert:
insert file in the navigation trail after the current page and go forward.

@mvr: pointer to #MtxViewer instance.
@file: path.
@offset: cursor offset at the jumping page.
*/
static void
_nav_trail_insert (MtxViewer *mvr,
                   const gchar *file,
                   guint offset)
{
    MtxViewerNavUnit *page = g_malloc (sizeof (MtxViewerNavUnit));

    if (mvr->nav_trail_page != NULL)
    {
        ((MtxViewerNavUnit *) mvr->nav_trail_page)->offset = offset;
    }
    page->offset = 0;
    page->file = g_strdup (file);
    g_queue_insert_after (mvr->nav_trail, g_queue_peek_nth_link
                          (mvr->nav_trail, mvr->nav_trail_page_idx), page);
    mvr->nav_trail_page = (gpointer) page;
    _nav_trail_fore (mvr); /* increments mvr->nav_trail_page_idx */
}

/**
_nav_trail_fore_clear
Clear the navigation trail forward of the current page.

@mvr: pointer to #MtxViewer instance
*/
static void
_nav_trail_fore_clear (MtxViewer *mvr)
{
    gpointer p;

    if (!mvr->can_go_fore)
    {
        return;
    }
    for (gint i = g_queue_get_length (mvr->nav_trail) - 1;
         i > mvr->nav_trail_page_idx; i--)
    /* for (gint i = 0; i < mvr->nav_trail_page_idx; i++) */
    {
        /* p = g_queue_pop_head (mvr->nav_trail); */
        p = g_queue_pop_tail (mvr->nav_trail);
        g_free (((MtxViewerNavUnit *)p)->file);
    }
    mvr->nav_trail_page_idx = g_queue_get_length (mvr->nav_trail) - 1;
    gtk_widget_set_sensitive (mvr->btn_nav_fore, mvr->can_go_fore = FALSE);
}

/**
_scroll_to_curpos:

@curpos: #GtkTextBuffer offset.
@highlight:
*/
static void
_scroll_to_curpos (MtxViewer *mvr,
                   const gint curpos,
                   const MtxTextViewHilightMode highlight)
{
    GtkTextIter iter;
    GtkTextMark *mark;
    MtxTextView *tv = MTX_TEXT_VIEW (mvr->text_view);
#ifdef MTX_DEBUG
#define mtx_dbg_get_curpos(LEVEL, FMTPREFIX)                           \
    do {                                                               \
        gint cp;                                                       \
        g_object_get (tv->buffer, "cursor-position", &cp, NULL);       \
        mtx_dbg_errseq (LEVEL, FMTPREFIX "(%d)", cp);                  \
    } while (0);
#endif

    mtx_dbg_errout (-1, "A \"%s\" curpos(%d)", mvr->current_file, curpos);
    gtk_text_buffer_get_iter_at_offset (tv->buffer, &iter, curpos);
    gtk_text_buffer_place_cursor (tv->buffer, &iter);
    mark = gtk_text_buffer_get_insert (tv->buffer);
    gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (tv), mark, 0.0, TRUE, 0.0,
                                  0.5);
    if (curpos > 0 && highlight)
    {
#ifdef MTX_DEBUG
        mtx_dbg_get_curpos (-1, " => B");
#endif
        mtx_text_view_highlight_at_cursor (MTX_TEXT_VIEW
                                           (mvr->text_view), highlight);
    }
#ifdef MTX_DEBUG
    mtx_dbg_get_curpos (-1, " => Z");
    mtx_dbg_errseq(0, "%c", '\n');
#undef mtx_dbg_get_curpos
#endif
}

static gboolean
idle_scroll_to_curpos (MtxViewer *mvr)
{
    _scroll_to_curpos (mvr, mvr->current_curpos,
                       MTX_TEXT_VIEW_HILIGHT_NORMAL);
    return FALSE;
}

/**
open_url:
Open a URL with a suitable Web browser.

Web browser search order (first match wins):
$BROWSER, xdg-open, Gnome open, KDE open,
a list of graphical Web browsers, links -g.
*/
static void
open_url (MtxViewer *mvr,
          const gchar *url)
{
    const gchar *browsers[] =
    {
        "xdg-open", "gnome-open", "kfmclient openURL",
        "sensible-browser", "firefox", "epiphany",
        "iceweasel", "seamonkey", "galeon", "mozilla",
        "opera", "konqueror", "netscape", "vivaldi",
        "links -g", NULL
    };
    gint i = 0;
    gchar *browser = (gchar *) g_getenv ("BROWSER");

    if (browser == NULL || *browser == '\0')
    {
        browser = (gchar *) browsers[i++];
    }
    do {
        gchar *cmdline = g_strdup_printf ("%s '%s'", browser, url);

        if (g_spawn_command_line_async (cmdline, NULL))
        {
            g_free (cmdline);
            return;
        }
        g_free (cmdline);
        browser = (gchar *) browsers[i++];
    } while (browser != NULL);
    {
        gchar *message =
        g_strdup (_("Web browser not found. Set environment variable BROWSER."));
        gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_WARN,
                            message);
        g_free (message);
    }
}

/**
edit_text_file:
Edit a file using the default editor.

Default editor search order (first match wins):
$DEFAULTTEXTEDITOR, defaulttexteditor,
a list of graphical text editors.
*/
static void
edit_text_file (MtxViewer *mvr,
                const gchar *file)
{
    const gchar *editors[] =
    {
        "defaulttexteditor", "gedit", "kate", "leafpad", "mousepad", "pluma",
        NULL
    };
    gint i = 0;
    gchar *editor = (gchar *) g_getenv ("DEFAULTTEXTEDITOR");

    if (editor == NULL || *editor == '\0')
    {
        editor = (gchar *) editors[i++];
    }
    do {
        gchar *cmdline = g_strdup_printf ("%s '%s'", editor, file);

        if (g_spawn_command_line_async (cmdline, NULL))
        {
            g_free (cmdline);
            return;
        }
        g_free (cmdline);
        editor = (gchar *) editors[i++];
    } while (editor != NULL);
    {
        gchar *message =
        g_strdup (_("Text editor not found. Set environment variable DEFAULTTEXTEDITOR."));
        gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_WARN,
                            message);
        g_free (message);
    }
}

/**
dispatch_to_page:
Load a new page from a file path or a URI.
Supported URIs: `resource://...`, `search://...`.
on_link_clicked() handles URI schemes `http:`, `https:`, `ftp:` and `mailto:`.

Returns: TRUE if a new page was loaded otherwise returns FALSE.
*/
/*
A discipline of navigation history:
Viewer navigation history shall be managed only by the callers of this function
and by the `file_load_complete` callback, which is connected to the
"file-load-complete" signal emitted by `mtx_text_view_load_file()`.
*/
static gboolean
dispatch_to_page (MtxViewer *mvr,
                  const gchar *path)
{
    const gchar *scheme = g_uri_peek_scheme (path);
    gboolean retval = FALSE; /* => found an unknown scheme */

    if (g_strcmp0 (scheme, "search") == 0)
    {
        retval = do_file_search (mvr, path + sizeof ("search://") - 1);
        if (retval)
        {
            file_load_complete (MTX_TEXT_VIEW (mvr->text_view), path, mvr);
        }
    }
    else if (g_strcmp0 (scheme, "resource") == 0)
    {
        const gchar *p = path + sizeof "resource://" - 1;
        retval = do_resource_load (mvr, p, p);
        if (retval)
        {
            file_load_complete (MTX_TEXT_VIEW (mvr->text_view), path, mvr);
        }
    }
    else if (scheme == NULL)
    {
        retval =
        mtx_text_view_load_file (MTX_TEXT_VIEW (mvr->text_view),
                                 path,
                                 mvr->current_file ==
                                 NULL ? "" : mvr->current_file, TRUE);
    }
    else
    {
        /* TODO status bar message telling scheme+path can't load */
    }
    g_idle_add (G_SOURCE_FUNC (idle_scroll_to_curpos), mvr);
    return retval;
}

/**
*/
static void
nav_fore_clicked (GtkWidget *widget __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxViewerNavUnit *fore =
    (MtxViewerNavUnit *) g_queue_peek_nth (mvr->nav_trail,
                                           mvr->nav_trail_page_idx + 1);
    guint saved_curpos = mvr->current_curpos;
    ((MtxViewerNavUnit *) mvr->nav_trail_page)->offset = mvr->changed_curpos;
    mvr->current_curpos = fore->offset;
    if (dispatch_to_page (mvr, fore->file))
    {
        _nav_trail_fore (mvr);
    }
    else
    {
        mvr->current_curpos = saved_curpos;
    }
}

/**
*/
static gboolean
accel_nav_fore (GtkAccelGroup *group __attribute__((unused)),
                GObject *obj __attribute__((unused)),
                guint *keyval __attribute__((unused)),
                GdkModifierType mod __attribute__((unused)),
                gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    if (mvr->can_go_fore)
    {
        nav_fore_clicked (NULL, data);
    }
    return TRUE;
}

/**
*/
static void
nav_back_clicked (GtkWidget *widget __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxViewerNavUnit *back =
    (MtxViewerNavUnit *) g_queue_peek_nth (mvr->nav_trail,
                                           mvr->nav_trail_page_idx - 1);
    guint saved_curpos = mvr->current_curpos;
    ((MtxViewerNavUnit *) mvr->nav_trail_page)->offset = mvr->changed_curpos;
    mvr->current_curpos = back->offset;
    if (dispatch_to_page (mvr, back->file))
    {
        _nav_trail_back (mvr);
    }
    else
    {
        mvr->current_curpos = saved_curpos;
    }
}

/**
*/
static gboolean
accel_nav_back (GtkAccelGroup *group __attribute__((unused)),
                GObject * obj __attribute__((unused)),
                guint keyval __attribute__((unused)),
                GdkModifierType mod __attribute__((unused)),
                gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    if (mvr->can_go_back)
    {
        nav_back_clicked (NULL, data);
    }
    return TRUE;
}

/**
on_link_clicked:
Callback from #MtxTextView class.
@link_dest: format: <uri-encoded>\n<verbatim>
*/
static void
on_link_clicked (MtxTextView *text_view,
                 const gchar *link_dest,
                 gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *end = strchr (link_dest, '\n');       /* uri-encoded */
    g_assert (end != NULL);
    gchar *link = g_strndup (link_dest, end - link_dest);
    const gchar *scheme = g_uri_peek_scheme (link);
    const gchar *current_scheme = g_uri_peek_scheme (mvr->current_file);

    if (scheme == NULL)
    {
        gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK);
        const guint offset = mvr->changed_curpos;
        if (mtx_text_view_load_file (text_view, link, current_scheme ? "/" :
                                     mvr->current_file, TRUE))
        {
            _nav_trail_fore_clear (mvr);
            _nav_trail_insert (mvr, mvr->current_file, offset);
            mtx_text_view_cursor_to_top (MTX_TEXT_VIEW (mvr->text_view));
        }
    }
    else if (strcmp (scheme, "file") == 0)
    {
        gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK);
        const guint offset = mvr->changed_curpos;
        if (mtx_text_view_load_file (text_view, link + sizeof "file://" - 1,
                                     current_scheme ? "/" :
                                     mvr->current_file, TRUE))
        {
            _nav_trail_fore_clear (mvr);
            _nav_trail_insert (mvr, mvr->current_file, offset);
            mtx_text_view_cursor_to_top (MTX_TEXT_VIEW (mvr->text_view));
        }
    }
    else if (strcmp (scheme, "https") == 0 || strcmp (scheme, "http") == 0
             || strcmp (scheme, "ftp") == 0 || strcmp (scheme, "mailto") == 0)
    {
        open_url (mvr, link);
    }
    g_free (link);
}

/**
file_load_complete:
Callback from #MtxTextView class and, in some cases, called directly by
#dispatch_to_page.
*/
static void
file_load_complete (MtxTextView *text_view __attribute__((unused)),
                    const gchar *file,
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *message = NULL;
    const gchar *scheme = g_uri_peek_scheme (file);

    if (g_strcmp0 (scheme, "search") == 0)
    {
        message = g_strdup (_("Search complete."));
    }
    else if (g_strcmp0 (scheme, "resource") == 0)
    {
        message = g_strdup (_("Loaded."));
    }
    else if (scheme == NULL)
    {
        gchar *p = g_path_get_basename (file);
        message = g_strdup_printf (_("%1$s loaded."), p);
        g_free (p);
    }

    /* Set the currently-loaded file. */
    g_free (mvr->current_file);
    mvr->current_file = g_strdup (file);

    gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK);
    gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_WARN);
    gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_MAIN,
                        message);
    g_free (message);
}

/**
on_curpos_changed:
*/
static void
on_curpos_changed (GtkTextBuffer *buffer,
                   GParamSpec *a2 __attribute__((unused)),
                   gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    g_object_get (buffer, "cursor-position", &mvr->changed_curpos, NULL);
    mtx_dbg_errout (-1, "changed_curpos (%d)\n", mvr->changed_curpos);
}

/**
hovering_over_link:
Callback from #MtxTextView class.
*/
static void
hovering_over_link (MtxTextView *text_view __attribute__((unused)),
                    const gchar *link,
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *temp;

    temp = g_strdup_printf (_("Link to %s"), link);
    gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK,
                        temp);
    g_free (temp);
}

/**
hovering_over_text:
Callback from #MtxTextView class.
*/
static void
hovering_over_text (MtxTextView *text_view __attribute__((unused)),
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;

    gtk_statusbar_pop (GTK_STATUSBAR (mvr->status_bar), STATUSBAR_CTX_LINK);
}

/**
_file_get_content_type:

Returns: newly-allocated string, NULL on error, "" on content type unknown,
otherwise the content type string.
*/
static gchar *
_file_get_content_type (const gchar *path)
{
    const gchar *content_type;
    GError *error = NULL;
    g_autoptr (GFile) file = g_file_new_for_path (path);
    g_autoptr (GFileInfo) info =
    g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL,
                       &error);
    if (error != NULL)
    {
        g_error_free (error);
        return NULL;
    }
    if (info == NULL)
    {
        return NULL;
    }
    content_type = g_file_info_get_content_type (info);
    return g_strdup (content_type == NULL ? "" : content_type);
}

/**
_is_text_and_markdown:

@content_type: MIME-type string.
@is_markdown: pointer to #gboolean, NULLABLE.

Returns TRUE if MIME type is text then sets *is_markdown if MIME subtype is
markdown. Otherwise it returns FALSE and *is_boolean is untouched.
*/
static gboolean
_is_text_and_markdown (const gchar *content_type,
                       gboolean *is_markdown)
{
    if (strncmp (content_type, "text/", sizeof ("text/") - 1) == 0)
    {
        if (is_markdown != NULL)
        {
            /* https://stackoverflow.com/a/25812177 */
            *is_markdown =
            strstr (content_type + sizeof ("text/") - 1, "markdown") != NULL;
        }
        return TRUE;
    }
    return FALSE;
}

/**
_build_search_lists:

Build two lists of searchable files in the homepage directory.  Only text files
are considered. If auto_languages is active, results will prefer File.$LANG.ext
over File.ext.

@markdown: address of a #GSList pointer that will received the list of markdown
files in the directory.
@text: address of a #GSList pointer that will received the list of other text
files in the directory.

Returns: the total number of elements in the two lists or -1 if case of error.
List elements can be NULL. *@markdown and @text are NULL if a list is empty.
The caller owns the returned lists and should free them when done.
*/
static gint
_build_search_lists (MtxViewer *mvr,
                     GSList **markdown,
                     GSList **text)
{
    GDir *dir;
    const gchar *name;
    guint counter = 0;
    g_autofree gchar *abs_dirpath =
    g_canonicalize_filename (mvr->base_directory, NULL);

    *markdown = *text = NULL;
    if ((dir = g_dir_open (abs_dirpath, 0, NULL)) == NULL)
    {
        return -1;
    }
    while ((name = g_dir_read_name (dir)))
    {
        gboolean is_text_type = FALSE;
        gboolean is_text_markdown = FALSE;
        gchar *path = g_build_filename (abs_dirpath, name, NULL);
        gchar *content_type = _file_get_content_type (path);

        if (content_type == NULL)
        {
            g_free (path);
            continue;
        }
        is_text_type =
        _is_text_and_markdown (content_type, &is_text_markdown);
        g_free (content_type);
        if (is_text_type)
        {
            GSList **head = is_text_markdown ? markdown : text;
            gchar *filename = NULL;
            if (mvr->auto_lang)
            {
                filename =
                mtx_text_view_auto_lang_find (MTX_TEXT_VIEW
                                              (mvr->text_view), path);
            }
            if (filename == NULL)
            {
                filename = path;
            }
            *head = g_slist_insert (*head, (gpointer) filename, 0);
            ++counter;
        }
    }
    g_dir_close (dir);
    return counter;
}

/**
_file_search:
*/
static void
_file_search (gpointer path, gpointer pod)
{
    typedef struct
    {
        gboolean is_text_markdown;
        GString *retstr;
        gchar **terms;
        guint *ctr;
        GRegex *regex_astx;
        GtkEntry *entry;
    } POD;
    POD *ppod = (POD *) pod;
    const gboolean is_text_markdown = ppod->is_text_markdown;
    GString *retstr    = ppod->retstr;
    gchar **terms      = ppod->terms;
    guint *counter     = ppod->ctr;
    GRegex *regex_astx = ppod->regex_astx;
    GtkEntry *entry    = ppod->entry;
    gboolean found = FALSE;

    gtk_entry_progress_pulse (entry);
    errno = 0;
    g_autofree gchar *contents = _get_file_contents (path, NULL, TRUE);
    if (contents == NULL)
    {
        if (errno)
        {
            fprintf (stderr, PROGNAME ": ");
            perror (path);
        }
        return;
    }
    if (contents[0] == '\0')
    {
        return;
    }
#ifdef _GNU_SOURCE
    for (guint term = 0; !found && terms[term]; term++)
    {
        found = strcasestr (contents, terms[term]) != NULL;
    }
#else
    gchar *upper1 = g_utf8_strup (contents, -1);
    for (guint term = 0; !found && terms[term]; term++)
    {
        gchar *upper2 = g_utf8_strup (terms[term], -1);
        found = strstr (upper1, upper2) != NULL;
        g_free (upper2);
    }
    g_free (upper1);
#endif
    if (found)
    {
        *counter +=1;
        /*
        Extract the page title from the first level-1 setext heading
        */
        GString *title = NULL, *dest = NULL;
        g_autoptr (GMatchInfo) minfo = NULL;

        if (is_text_markdown
            && g_regex_match (regex_astx, contents, 0, &minfo))
        {
            g_autofree gchar *p =
            g_match_info_fetch_named (minfo, "TITLE");
            title = g_string_new (p);
        }

        /* Sanitize title and destination. */
        if (title == NULL)
        {
            gchar *p;
            for (p = strchr (path, '\0'); p >= (gchar *) path; p--)
            {
                if (G_IS_DIR_SEPARATOR (*p))
                {
                    p++;
                    break;
                }
            }
            title = g_string_new (p);
        }
        title->str = g_strstrip (g_strdelimit
                          (title->str, "\\\n\r", ' '));
        g_string_set_size (title, strlen (title->str));
        g_string_replace (title, "]", "\\]", -1);
        dest = g_string_new (path);
        g_string_replace (dest, ")", "\\)", -1);

        g_string_append_printf (retstr, "* [%s](%s)\n",
                                title->str, dest->str);
        g_string_free (title, TRUE);
        g_string_free (dest, TRUE);
    }
}

/**
do_file_search:
Load the results of a search URI into a new viewing page.

Returns: TRUE if the new page was generated otherwise returns FALSE.
*/
/*
Result is a synthetic page.
We must not call mtx_text_view_load_file!
*/
static gboolean
do_file_search (MtxViewer *mvr,
                const gchar *text)
{
    GString *markdown = g_string_new (NULL);
    gchar *stripped, **terms;
    gint ctr_subjects, ctr_results = 0;
    GSList *mkd = NULL, *txt = NULL;
    GtkEntry *entry = GTK_ENTRY (mvr->text_search);
    gint argc = 0;

    stripped = g_strstrip (g_strdup (text));
    if (!g_shell_parse_argv (text, &argc, &terms, NULL))
    {
        terms = g_strsplit (stripped, " ", 0);
        argc = g_strv_length (terms);
    }
    g_free (stripped);
    ctr_subjects = _build_search_lists (mvr, &mkd, &txt);
    if (ctr_subjects < 0)
    {
        return FALSE;
    }

    gtk_widget_set_sensitive (mvr->window, FALSE);
    gtk_entry_set_progress_fraction (entry, 1.0f / (ctr_subjects + 1));
    gtk_entry_progress_pulse (entry);

    mkd = g_slist_sort (mkd, (GCompareFunc) g_strcmp0);
    txt = g_slist_sort (txt, (GCompareFunc) g_strcmp0);

    typedef struct
    {
        gboolean is_text_markdown;
        GString *retstr;
        gchar **terms;
        gint *ctr;
        GRegex *regex_astx;
        GtkEntry *entry;
    } POD;
    POD pod = { TRUE, markdown, terms, &ctr_results, mvr->regex_astx, entry };

    g_slist_foreach (mkd, (GFunc) _file_search, &pod);
    g_slist_free_full (mkd, g_free);
    pod.is_text_markdown = FALSE;
    g_slist_foreach (txt, (GFunc) _file_search, &pod);
    g_slist_free_full (txt, g_free);

    /* prepend formatted page heading */
    {
        guint n = 0;
        for (const gchar *p = text; *p; p++)
        {
            if (*p == '`') ++n;
        }
        g_autofree gchar *ctr_subjects_str =
        g_strdup_printf (ngettext
                         (_("%d document examined"),
                          _("%d documents examined"), ctr_subjects),
                         ctr_subjects);
        g_autofree gchar *codespan = g_strnfill (n + 1, '`');
        g_autofree gchar *ctr_results_str =
        g_strdup_printf (ngettext
                         (_("%d document found"),
                          _("%d documents found"), ctr_results),
                         ctr_results);
        g_autofree gchar *terms_str =
        g_strdup_printf (ngettext
                         (Q_ ("search:1=#terms:2=terms:3=```|%1$d search term %3$s %2$s %3$s"),
                          Q_ ("search:1=#terms:2=terms:3=```|%1$d search terms %3$s %2$s %3$s"),
                          argc), argc, text, codespan);
        g_autofree gchar *heading =
        g_strdup_printf (Q_("search:1=ctr_subjects,2=ctr_results:3=terms|### %1$s, %2$s, %3$s\n# \n"),
                         ctr_subjects_str, ctr_results_str, terms_str);
        g_string_prepend (markdown, heading);
    }
    /* show the results inside the textview */
    gboolean retval =
    mtx_text_view_set_text (MTX_TEXT_VIEW (mvr->text_view),
                            &markdown->str, NULL, TRUE);
    g_string_free (markdown, FALSE);

    gtk_entry_set_progress_fraction (entry, 0.0f);
    gtk_widget_set_sensitive (mvr->window, TRUE);
    g_strfreev (terms);
    return retval;
}

/**
do_resource_load:
Load the results of a resource URI into a new viewing page.

@mvr: The #MtxTextView instance.
@path: A disk file path. NULLABLE
@uri: An embedded resource file path. NULLABLE

First @path is looked in $XDG_USER_DATE:$XDG_DATA_DIRS, if that fails
then the embedded @uri is used.

Returns: TRUE if the new page was generated otherwise it returns FALSE.
*/
static gboolean
do_resource_load (MtxViewer *mvr,
                  const gchar *path,
                  const gchar *embed)
{
    gboolean retval = FALSE;

    /* Possibly load a disk file. */
    if (path != NULL)
    {
        g_autofree gchar *file = NULL;
        g_autofree gchar *contents = NULL;

        for (const gchar * const *p = mvr->data_dirs; *p; p++)
        {

            file = g_build_filename (*p, PROGNAME, path, NULL);
            contents =
            mtx_text_view_get_file_contents (MTX_TEXT_VIEW (mvr->text_view),
                                             file, NULL, TRUE);
            if (contents != NULL)
            {
                retval = mtx_text_view_set_text (MTX_TEXT_VIEW (mvr->text_view),
                                                 &contents, file, TRUE);
                break;
            }
            g_free (file);
            file = NULL;
        }
    }

    /* Fall back to loading an embedded file. */
    /* Must not call mtx_text_view_get_file_contents!  */

    if (!retval && embed != NULL)
    {
        g_autoptr (GBytes) bytes =
        g_resources_lookup_data (embed, 0, NULL);
        const gchar *contents = (const gchar *) g_bytes_get_data (bytes, NULL);
        if (contents != NULL)
        {
            retval = mtx_text_view_set_text (MTX_TEXT_VIEW (mvr->text_view),
                                             (gchar **) &contents, "/", FALSE);
        }
    }
    return retval;
}

/**
do_open_welcome_page:
Load the welcome page.

@mvr: The #MtxTextView instance.

Returns: TRUE if the help page was opened.
*/
static gboolean
do_open_welcome_page (MtxViewer *mvr)
{
    gboolean retval;
    const gchar *page = "resource:///welcome.md";
    const guint offset = mvr->current_curpos = 0;

    retval = dispatch_to_page (mvr, page);
    if (retval)
    {
        /*
        For consistency with the way the search:// URI in search_entry_activate
        is presented, here I prefer not to clear the fore trail before inserting
        the resource:// URI.
        */

        _nav_trail_insert (mvr, page, offset);
    }
    else
    {
        g_autofree gchar *temp =
        g_strdup_printf (("'%s': resource not found"), page);
        gtk_statusbar_push (GTK_STATUSBAR (mvr->status_bar),
                            STATUSBAR_CTX_WARN, temp);
    }
    return retval;
}

/**
*/
static void
search_entry_activate (GtkEntry *entry, gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    const gchar *needle = gtk_entry_get_text (entry);

    if (*needle)
    {
        g_autofree gchar *uri = NULL;
        const gchar *scheme = g_uri_peek_scheme (needle);
        const guint offset = mvr->changed_curpos;

        if (scheme == NULL)
        {
            uri = g_strdup_printf ("search://%s", needle);
        }
        else
        {
            uri = g_strdup (needle);
        }
        if (dispatch_to_page (mvr, uri))
        {
            /*
            Unlike on_link_clicked and nav_home_clicked, which clear the fore
            trail on dispatch_to_page success, in this case I prefer to insert
            the search results page in the trail after the current page, without
            clearing the fore trail.
            */

            _nav_trail_insert (mvr, uri, offset);
        }
    }
    else
    {
        /* takes care of managing the navigation history */
        (void) do_open_welcome_page (mvr);
    }
    gtk_widget_grab_focus (GTK_WIDGET (mvr->text_search));
}

/**
*/
static void
search_entry_icon_press (GtkEntry *entry,
                         gint position,
                         GdkEventButton *event,
                         gpointer data)
{
    if (position == GTK_ENTRY_ICON_PRIMARY)
    {
        search_entry_activate (entry, data);
    }
    else
    {
        MtxViewer *mvr = (MtxViewer *) data;
        const gchar *needle = gtk_entry_get_text (entry);

        if (*needle)
        {
            MtxTextViewSearchOptions options =
            MTX_TEXT_VIEW_SEARCH_HILIGHT | MTX_TEXT_VIEW_SEARCH_ONE_HILIGHT;
            event->button &= ~0x1000;
            options |= (event->button == 1 ? MTX_TEXT_VIEW_SEARCH_FORE :
                MTX_TEXT_VIEW_SEARCH_BACK);
            (void) mtx_text_view_find_text (MTX_TEXT_VIEW (mvr->text_view),
                                            needle, options);
        }
        else
        {
            search_entry_activate (entry, data);
        }
        gtk_widget_grab_focus (GTK_WIDGET (mvr->text_search));
    }
}

/**
*/
static gboolean
accel_search_entry_focus (GtkAccelGroup *group __attribute__((unused)),
                          GObject *obj __attribute__((unused)),
                          guint keyval __attribute__((unused)),
                          GdkModifierType mod __attribute__((unused)),
                          gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;

    gtk_widget_grab_focus (GTK_WIDGET (mvr->text_search));
    return TRUE;
}

/**
*/
static gboolean
accel_search_fore (GtkAccelGroup *group __attribute__((unused)),
                   GObject *obj __attribute__((unused)),
                   guint keyval __attribute__((unused)),
                   GdkModifierType mod __attribute__((unused)),
                   gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    GdkEventButton e;

    e.button = 1 | 0x1000;
    search_entry_icon_press (GTK_ENTRY (mvr->text_search),
                             GTK_ENTRY_ICON_SECONDARY, &e, mvr);
    return TRUE;
}

/**
*/
static gboolean
accel_search_back (GtkAccelGroup *group __attribute__((unused)),
                   GObject *obj __attribute__((unused)),
                   guint keyval __attribute__((unused)),
                   GdkModifierType mod __attribute__((unused)),
                   gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    GdkEventButton e;

    e.button = 3 | 0x1000;
    search_entry_icon_press (GTK_ENTRY (mvr->text_search),
                             GTK_ENTRY_ICON_SECONDARY, &e, mvr);
    return TRUE;
}

/**
*/
static void
nav_home_clicked (GtkWidget *button __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *homepage = mvr->homepage == NULL ? DEFAULT_INDEX : mvr->homepage;
    const guint offset = mvr->changed_curpos;

    if (dispatch_to_page (mvr, homepage))
    {
        _nav_trail_fore_clear (mvr);
        _nav_trail_insert (mvr, homepage, offset);
    }
}

/**
*/
static gboolean
accel_nav_home (GtkAccelGroup *group __attribute__((unused)),
                GObject *obj __attribute__((unused)),
                guint keyval __attribute__((unused)),
                GdkModifierType mod __attribute__((unused)),
                gpointer data)
{
    nav_home_clicked (NULL, data);
    return TRUE;
}

/**
scroll_to_link_and_highlight:
Scroll the page to a given %MtxCmmTagInfo and highlight the link text.

@mvr:
@link_info: pointer to %MtxTextViewLinkInfo.
*/
static void
scroll_to_link_and_highlight (MtxViewer *mvr,
                              const MtxTextViewLinkInfo *link_info)
{
    GtkTextIter iter;
    MtxTextView *tv = MTX_TEXT_VIEW (mvr->text_view);
    gtk_text_buffer_get_iter_at_mark (tv->buffer, &iter, link_info->mark);
    gtk_text_buffer_place_cursor (tv->buffer, &iter);
    mtx_text_view_clear_search_highlights (tv);
    mtx_text_view_highlight_at_cursor_chars (tv, link_info->llen,
                                             MTX_TEXT_VIEW_HILIGHT_NORMAL);
    gtk_widget_grab_focus (GTK_WIDGET (tv));
    gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (tv), link_info->mark);
}

/**
*/
static gboolean
accel_link_fore (GtkAccelGroup *group __attribute__((unused)),
                 GObject *obj __attribute__((unused)),
                 guint keyval __attribute__((unused)),
                 GdkModifierType mod __attribute__((unused)),
                 gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxTextView *tv = MTX_TEXT_VIEW (mvr->text_view);
    const MtxTextViewLinkInfo *p;

    p = mtx_text_view_link_info_get_near_offset (tv, mvr->changed_curpos, +1);
    if (p != NULL)
    {
        scroll_to_link_and_highlight (mvr, p);
    }
    return TRUE;
}

/**
*/
static gboolean
accel_link_back (GtkAccelGroup *group __attribute__((unused)),
                 GObject *obj __attribute__((unused)),
                 guint keyval __attribute__((unused)),
                 GdkModifierType mod __attribute__((unused)),
                 gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxTextView *tv = MTX_TEXT_VIEW (mvr->text_view);
    const MtxTextViewLinkInfo *p;

    p = mtx_text_view_link_info_get_near_offset (tv, mvr->changed_curpos, -1);
    if (p != NULL)
    {
        scroll_to_link_and_highlight (mvr, p);
    }
    return TRUE;
}

/**
*/
static gboolean
accel_open_help (GtkAccelGroup *group __attribute__((unused)),
                 GObject *obj __attribute__((unused)),
                 guint keyval __attribute__((unused)),
                 GdkModifierType mod __attribute__((unused)),
                 gpointer data)
{
    (void) do_open_welcome_page ((MtxViewer *) data);
    return TRUE;
}

/**
*/
static gboolean
accel_edit_current (GtkAccelGroup *group __attribute__((unused)),
                    GObject *obj __attribute__((unused)),
                    guint keyval __attribute__((unused)),
                    GdkModifierType mod __attribute__((unused)),
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxTextView *tv = (MtxTextView *) mvr->text_view;
    gchar *file_name = mvr->current_file;

    if (file_name != NULL)
    {
        if (file_name[0] == '/')
        {
            edit_text_file (mvr, file_name);
        }
        else
        {
            gchar *path =
            g_build_filename (tv->image_directory, file_name, NULL);
            edit_text_file (mvr, path);
            g_free (path);
        }
    }
    return TRUE;
}

/**
mtx_viewer_present_page:
Present a file or supported URI.

@mvr: pointer to #MtxViewer.
@page: filepath or supported URI.

Returns: TRUE if the page is presented otherwise returns FALSE after displaying
an error dialog.
*/
gboolean
mtx_viewer_present_page (MtxViewer *mvr,
                          const gchar *page,
                          guint offset)
{
    gboolean retval = dispatch_to_page (mvr, page);
    if (retval)
    {
        _nav_trail_fore_clear (mvr);
        _nav_trail_insert (mvr, mvr->current_file, offset);
        gtk_window_present (GTK_WINDOW (mvr->window));
    }
    else
    {
        GtkWidget *dialog =
        gtk_message_dialog_new (GTK_WINDOW (mvr->parent),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_ERROR,
                                GTK_BUTTONS_CLOSE,
                                _("Cannot open '%s'."), page);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }
    return retval;
}

/**
*/
static void
_nav_unit_clear (MtxViewerNavUnit *unit,
                 MtxViewer *mvr __attribute__((unused)))
{
    g_free (unit->file);
    g_free (unit);
}

/**
mtx_viewer_destroy:
*/
void
mtx_viewer_destroy (MtxViewer *mvr)
{
    if (mvr->nav_trail != NULL)
    {
        g_queue_foreach (mvr->nav_trail, (GFunc) _nav_unit_clear, mvr);
        g_queue_clear (mvr->nav_trail);
        g_queue_free (mvr->nav_trail);
        mvr->nav_trail = NULL;
    }

    g_free (mvr->current_file);
    g_free (mvr->base_directory);
    g_free ((gpointer) mvr->data_dirs);
    if (mvr->regex_astx != NULL)
    {
        g_regex_unref (mvr->regex_astx);
    }
    if (mvr->parent == NULL && gtk_main_level ())
    {
        gtk_main_quit ();
    }
}

/**
*/
static gboolean
viewer_destroy_me (GtkWidget *widget __attribute__((unused)),
                   gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;

    mtx_viewer_destroy (mvr);
    return FALSE;
}

static gboolean
viewer_key_pressed (GtkWidget *widget,
                    GdkEventKey *event,
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;

    switch (event->keyval)
    {
        case GDK_KEY_Escape:
            mtx_viewer_destroy (mvr);
            gtk_widget_destroy (widget);
            return TRUE;
    }
    return FALSE;
}

/**
mtx_viewer_new:
*/
MtxViewer *
mtx_viewer_new (const gchar *base_dir,
                const gchar *base_file,
                const gchar *title,
                GtkWindow *parent,
                guint extensions,
                guint tweaks)
{
    MtxViewer *mvr;
    GtkWidget *mtx_viewer;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *toolbar1;
    GtkWidget *separatortoolitem1;
    GtkWidget *toolbar2;
    GtkWidget *toolitem3;
    GtkWidget *toolitem4;
    GtkWidget *search_entry;
    GtkWidget *scrolled_mtx_viewer;
    GtkWidget *mtx_text_view;
    GtkWidget *status_bar;
    GtkWidget *btn_nav_back, *btn_nav_fore, *btn_nav_home;
    GtkAccelGroup *accel;

    mtx_viewer = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (mtx_viewer, 300, 200);
    gtk_window_set_default_size (GTK_WINDOW (mtx_viewer), 640, 480);
    gtk_window_set_title (GTK_WINDOW (mtx_viewer),
                          title ? title : DEFAULT_WINDOW_TITLE);
    gtk_window_set_transient_for (GTK_WINDOW (mtx_viewer), parent);

#if !GTK_CHECK_VERSION(3,0,0)
    GdkPixbuf *icon = gtk_widget_render_icon (mtx_viewer, GTK_STOCK_HELP,
                                              GTK_ICON_SIZE_DIALOG, NULL);
    gtk_window_set_icon (GTK_WINDOW (mtx_viewer), icon);
    g_object_unref (icon);

    vbox = gtk_vbox_new (FALSE, 0);
#else
    gtk_window_set_icon_name (GTK_WINDOW (mtx_viewer), "help-browser");
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (mtx_viewer), vbox);

#if !GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_hbox_new (FALSE, 0);
#else
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    toolbar1 = gtk_toolbar_new ();
    gtk_widget_show (toolbar1);
    gtk_box_pack_start (GTK_BOX (hbox), toolbar1, TRUE, TRUE, 0);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_BOTH_HORIZ);

#if !GTK_CHECK_VERSION(3,0,0)
    btn_nav_home = (GtkWidget *) gtk_tool_button_new_from_stock ("gtk-home");
#else
    GtkWidget *icon_home =
    gtk_image_new_from_icon_name ("go-home", GTK_ICON_SIZE_LARGE_TOOLBAR);
    btn_nav_home = (GtkWidget *) gtk_tool_button_new (icon_home, _("Home"));
#endif
    gtk_widget_set_tooltip_text (btn_nav_home,
                                 _("(Alt-H) Reload the home page"));
    gtk_widget_show (btn_nav_home);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_nav_home);

#if !GTK_CHECK_VERSION(3,0,0)
    btn_nav_back = (GtkWidget *) gtk_tool_button_new_from_stock ("gtk-go-back");
#else
    GtkWidget *icon_previous =
    gtk_image_new_from_icon_name ("go-previous",
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);
    btn_nav_back = (GtkWidget *) gtk_tool_button_new (icon_previous, _("Back"));
#endif
    gtk_widget_set_tooltip_text (btn_nav_back,
                                 _("(Alt-B) Go back"));
    gtk_widget_show (btn_nav_back);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_nav_back);
    /* gtk_tool_item_set_is_important (GTK_TOOL_ITEM (btn_nav_back), TRUE); */
    gtk_widget_set_sensitive (btn_nav_back, FALSE);

#if !GTK_CHECK_VERSION(3,0,0)
    btn_nav_fore =
    (GtkWidget *) gtk_tool_button_new_from_stock ("gtk-go-forward");
#else
    GtkWidget *icon_next =
    gtk_image_new_from_icon_name ("go-next", GTK_ICON_SIZE_LARGE_TOOLBAR);
    btn_nav_fore = (GtkWidget *) gtk_tool_button_new (icon_next, _("Forward"));
#endif
    gtk_widget_set_tooltip_text (btn_nav_fore, _("(Alt-F) Go forward"));
    gtk_widget_show (btn_nav_fore);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_nav_fore);
    /* gtk_tool_item_set_is_important (GTK_TOOL_ITEM (btn_nav_fore), TRUE); */
    gtk_widget_set_sensitive (btn_nav_fore, FALSE);

    separatortoolitem1 = (GtkWidget *) gtk_separator_tool_item_new ();
    gtk_widget_show (separatortoolitem1);
    gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem1);

    toolbar2 = gtk_toolbar_new ();
    gtk_widget_show (toolbar2);
    gtk_box_pack_end (GTK_BOX (hbox), toolbar2, FALSE, TRUE, 0);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar2), FALSE);

    toolitem3 = (GtkWidget *) gtk_tool_item_new ();
    gtk_widget_show (toolitem3);
    gtk_container_add (GTK_CONTAINER (toolbar2), toolitem3);

    toolitem4 = (GtkWidget *) gtk_tool_item_new ();
    gtk_widget_show (toolitem4);
    gtk_container_add (GTK_CONTAINER (toolbar2), toolitem4);

    search_entry = gtk_entry_new ();
    gtk_widget_show (search_entry);
    gtk_container_add (GTK_CONTAINER (toolitem4), search_entry);
    gtk_entry_set_invisible_char (GTK_ENTRY (search_entry), 9679);
#if !GTK_CHECK_VERSION(3,0,0)
    gtk_entry_set_icon_from_stock (GTK_ENTRY (search_entry),
                                   GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_INDEX);
    gtk_entry_set_icon_from_stock (GTK_ENTRY (search_entry),
                                   GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FIND);
#else
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (search_entry),
                                       GTK_ENTRY_ICON_PRIMARY, "gtk-index");
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (search_entry),
                                       GTK_ENTRY_ICON_SECONDARY, "edit-find");
#endif
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (search_entry),
                                     GTK_ENTRY_ICON_PRIMARY,
                                     _("(Alt-S) Set focus on the search field"
                                       " to enter terms\n(Enter) Search"
                                       " through all documents"));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (search_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("(Ctrl-F) Search forward in this page "
                                       "(also by primary button click)\n"
                                       "(Ctrl-B) Search backward "
                                       "(also by opposite button click)"));

    scrolled_mtx_viewer = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
                                         (scrolled_mtx_viewer),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_mtx_viewer, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_mtx_viewer),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    mtx_text_view = mtx_text_view_new ();
    mtx_text_view_set_extensions (MTX_TEXT_VIEW (mtx_text_view), extensions);
    mtx_text_view_set_tweaks (MTX_TEXT_VIEW (mtx_text_view), tweaks);
    mtx_text_view_set_image_directory (MTX_TEXT_VIEW (mtx_text_view), base_dir);
    mtx_text_view_set_auto_lang_find (MTX_TEXT_VIEW (mtx_text_view),
                                          extensions &
                                          MTX_CMM_EXTENSION_AUTO_LANG);
    gtk_container_add (GTK_CONTAINER (scrolled_mtx_viewer), mtx_text_view);

    status_bar = gtk_statusbar_new ();
    gtk_widget_show (status_bar);
    gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);

    mvr = g_new0 (MtxViewer, 1);
    mvr->window = mtx_viewer;
    mvr->status_bar = status_bar;
    mvr->btn_nav_back = btn_nav_back;
    mvr->btn_nav_fore = btn_nav_fore;
    mvr->text_view = mtx_text_view;
    mvr->text_search = search_entry;
    mvr->base_directory = g_strdup (base_dir);
    mvr->nav_trail = g_queue_new ();
    mvr->nav_trail_page = NULL;
    mvr->nav_trail_page_idx = -1;
    mvr->parent = GTK_WIDGET (parent);
    mvr->can_go_back = mvr->can_go_fore = FALSE;
    mvr->auto_lang = extensions & MTX_CMM_EXTENSION_AUTO_LANG;

    g_signal_connect (mtx_viewer, "delete-event",
                      G_CALLBACK (viewer_destroy_me), mvr);
    g_signal_connect (mtx_viewer, "key-press-event",
                      G_CALLBACK (viewer_key_pressed), mvr);
    g_signal_connect (mtx_text_view, "link-clicked",
                      G_CALLBACK (on_link_clicked), mvr);
    g_signal_connect (mtx_text_view, "hovering-over-link",
                      G_CALLBACK (hovering_over_link), mvr);
    g_signal_connect (mtx_text_view, "hovering-over-text",
                      G_CALLBACK (hovering_over_text), mvr);
    g_signal_connect (mtx_text_view, "file-load-complete",
                      G_CALLBACK (file_load_complete), mvr);
    g_signal_connect (MTX_TEXT_VIEW(mtx_text_view)->buffer,
                      "notify::cursor-position",
                      G_CALLBACK (on_curpos_changed), mvr);
    g_signal_connect (btn_nav_back, "clicked", G_CALLBACK (nav_back_clicked),
                      mvr);
    g_signal_connect (btn_nav_fore, "clicked", G_CALLBACK (nav_fore_clicked),
                      mvr);
    g_signal_connect (btn_nav_home, "clicked", G_CALLBACK (nav_home_clicked),
                      mvr);
    g_signal_connect (search_entry, "activate",
                      G_CALLBACK (search_entry_activate), mvr);
    g_signal_connect (search_entry, "icon-press",
                      G_CALLBACK (search_entry_icon_press), mvr);

    accel = gtk_accel_group_new ();
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("F1"),
                             0, 0,
                             g_cclosure_new (G_CALLBACK (accel_open_help), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("s"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK
                                             (accel_search_entry_focus), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("f"),
                             GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_search_fore),
                                             mvr, NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("b"),
                             GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_search_back),
                                             mvr, NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("h"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_nav_home), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("f"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_nav_fore), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("b"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_nav_back), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("f"),
                             GDK_SHIFT_MASK | GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_link_fore), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("b"),
                             GDK_SHIFT_MASK | GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_link_back), mvr,
                                             NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("e"),
                             GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_edit_current),
                                             mvr, NULL));
    gtk_window_add_accel_group (GTK_WINDOW (mtx_viewer), accel);

    /* build data search path */
    {
        gchar **a, **p, *u;
        /* Glib owns a and u */
        a = (gchar **) g_get_system_data_dirs ();
        p = g_new (gchar *, g_strv_length (a) + 2);
        mvr->data_dirs = (const gchar * const *) p;
        u = (gchar *) g_get_user_data_dir ();
        if (*u)
        {
            *p++ = u;
        }
        while (*a)
        {
            *p++ = *a++;
        }
        *p = NULL;
    }
    /*
    Regex matcher for ATX and setext headings
    (the setext regex has some limitations as discussed in efe6f0f).
    */
    /* *INDENT-OFF* */
    mvr->regex_astx = g_regex_new (
    /* ATX or SETEX as TITLE  */ "(?|"
    /*       ATX              */ "(?:" /* ATX heading */
    /* start of line/file     */ "(?:\\R|^)"
    /* ATX heading signature  */ " {0,3}#{1,6}[ \\t]+"
    /* ~alphabetic title      */ "(?<TITLE>.*?(?:\\p{Ll}|\\p{Lu}).*?)"
    /* ATX optional ending    */ "#*\\s*?"
    /*                        */ ")|"
    /*      SETEX             */ "(?:" /* non-semantic setext heading */
    /* run of ...             */ "(?<TITLE>(?:"
    /* start of line/file     */ "(?:\\R|^)"
    /* no empty line, no list */ "(?: {0,3}[^-*\\s\\r`].*?))" /* no code */
    /*                        */ "+)"
    /* setext underlines      */ "(?:\\R|^) {0,3}[-=]+[ \\t]*"
    /*                        */ ")"
    /*                        */ ")"
    /* share end of line/file */ "(?:\\R|$)"
                                 , 0, 0, NULL);
    /* *INDENT-ON* */

    if (!mtx_viewer_present_page
        (mvr, base_file == NULL ? DEFAULT_INDEX : base_file, 0))
    {
        gtk_widget_destroy (mvr->window);
        mtx_viewer_destroy (mvr);
        g_free (mvr);
        return NULL;
    }

    gtk_widget_show_all (mvr->window);
    gtk_widget_grab_focus (mvr->text_view);
    return mvr;
}

