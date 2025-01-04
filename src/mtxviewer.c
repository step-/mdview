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
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_68
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
#include "mtxresources.h"
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

typedef struct
{
    MtxViewer *mvr;
} progress_logger_data;

typedef struct
{
    MtxViewer *mvr;
    int id;
} progress_logger_update_data;

#include "mtxviewer.decl.h"

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
    GString *message;

    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_LINK);
    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_WARN);
    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_MAIN);
    if (mvr->nav_trail_page == NULL)
    {
        return;
    }
    message = g_string_new ("");
    page = (MtxViewerNavUnit *) mvr->nav_trail_page;
    for (gint i = 0; i < mvr->nav_trail_page_idx; i++)
    {
        unit = g_queue_peek_nth (mvr->nav_trail, i);
        name = g_path_get_basename (unit->file);
        g_string_append_printf (message, " « %s", name);
        g_free (name);
    }
    if (mvr->nav_trail_page_idx == 0)
    {
        if (!mtx_viewer_is_current_tracked (mvr))
        {
            g_string_append (message, Q_
                             ("Home page accel, in status bar before the name|"
                              "(Alt-H)"));
        }
    }
    else
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
    gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN,
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
    if (mvr->nav_trail_page_idx <= 0)
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
Insert file in the navigation trail after the current page,
and go forward so that the file becomes the current page.

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
_nav_trail_fore_clear:
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
    MtxTextView *tv = mvr->text_view;
#ifdef MTX_DEBUG
#define mtx_dbg_get_curpos(LEVEL, FMTPREFIX)                           \
    do {                                                               \
        gint cp;                                                       \
        g_object_get (tv->buffer, "cursor-position", &cp, NULL);       \
        mtx_dbg_errseq (LEVEL, FMTPREFIX "(%d)", cp);                  \
    } while (0);
#endif

    if (highlight & MTX_TEXT_VIEW_HILIGHT_CLEAR_LINE)
    {
        if (mvr->landing_link_info != NULL)
        {
            mtx_text_view_clear_line_highlights (tv,
                                                 mvr->landing_link_info->mark);
        }
    }
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
        mtx_text_view_highlight_at_cursor (tv, highlight);
    }
#ifdef MTX_DEBUG
    mtx_dbg_get_curpos (-1, " => Z");
    mtx_dbg_errseq(-1, "%c", '\n');
#undef mtx_dbg_get_curpos
#endif
}

/**
idle_scroll_to_current_curpos:
To be called from g_idle_add only to ensure that scrolling is initiated
on the new page about to be loaded instead of on the current page.
*/
static gboolean
idle_scroll_to_current_curpos (MtxViewer *mvr)
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
    { "xdg-open", "gnome-open", "kfmclient openURL", NULL };
    gint i = 0;
    const gchar *browser = g_getenv ("BROWSER");

    if (browser == NULL || *browser == '\0')
    {
        browser = browsers[i++];
    }
    do {
        gchar *cmdline = g_strdup_printf ("%s '%s'", browser, url);

        if (g_spawn_command_line_async (cmdline, NULL))
        {
            g_free (cmdline);
            return;
        }
        g_free (cmdline);
        browser = browsers[i++];
    } while (browser != NULL);
    {
        gchar *message =
        g_strdup (_("Browser not found. Set environment variable BROWSER."));
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_WARN, message);
        g_free (message);
    }
}

/**
edit_text_file:
Edit a disk file using the default editor.

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
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_WARN, message);
        g_free (message);
    }
}

/**
mtx_viewer_save_backing_file:
Save content to **the** backing file.

Call this function to save the markdown text of the current page, which is
identified by a search:// or resource:// URI and does not correspond to a
disk file. It creates a temporary disk file for edit_text_file to open when
the user presses the Ctrl+E hotkey.

Return: TRUE on successful file write otherwise FALSE.
*/
static gboolean
mtx_viewer_save_backing_file (MtxViewer *mvr,
                              const gchar *buf,
                              const gsize size)
{
    gboolean retval = FALSE;

    if (mvr->backing_fd >= 0)
    {
        retval = g_file_set_contents (mvr->backing_file, buf, size, NULL);
    }
    return retval;
}
/****************************************************************************
*                          PROGRESS TRACKER TASKS                        {{{*
****************************************************************************/

static void progress_logger_async (GObject *, MtxViewer *, GCancellable *, GAsyncReadyCallback, gpointer);
static void progress_logger_thread_cb (GTask *, gpointer, gpointer, GCancellable *);
static gboolean progress_logger_update (progress_logger_update_data *);
static gint progress_logger_finish (GAsyncResult *, GError **);
static void progress_logger_completed (GObject *, GAsyncResult *, gpointer);

/**
progress_logger_start:
%GSourceFunc.
*/
static gboolean
progress_logger_start (MtxViewer *mvr)
{
    if (g_queue_get_length (mvr->progress_logger_q) == 0)
    {
        /* There should be at least one id, that is myself.
        Zero means that the page loaded quickly enough for
        progress_logger_stop to run ahead of me thus closing the gate. */
        return FALSE;
    }
    g_queue_pop_head (mvr->progress_logger_q);
    g_cancellable_reset (mvr->progress_logger_cancellable);
    /* Let's start */
    if (pipe (mvr->progress_fd) == -1)
    {
        g_warning ("%s: pipe: %s\n", PROGNAME, g_strerror (errno));
        return FALSE;
    }
    gtk_progress_bar_set_fraction (mvr->progress_bar, 0);
    gtk_progress_bar_set_text (mvr->progress_bar, _("Working..."));
    gtk_widget_set_visible (mvr->progress_box, TRUE);
    progress_logger_async (NULL, mvr,
                           mvr->progress_logger_cancellable,
                           (GAsyncReadyCallback) progress_logger_completed,
                           NULL);
    mtx_text_view_set_progress_fd (mvr->text_view, mvr->progress_fd[1]);
    return FALSE;
}

/**
progress_logger_stop:
*/
static void
progress_logger_stop (MtxViewer *mvr)
{
    gpointer *p;
    while ((p = g_queue_pop_head (mvr->progress_logger_q)))
    {
        g_source_remove (GPOINTER_TO_UINT (p));
    }
    g_cancellable_cancel (mvr->progress_logger_cancellable);
    gtk_widget_set_visible (mvr->progress_box, FALSE);
    mtx_text_view_set_progress_fd (mvr->text_view, -1);
    close (mvr->progress_fd [0]);
    close (mvr->progress_fd [1]);
    mvr->progress_fd[0] = mvr->progress_fd[1] = 0;
}

/**
progress_logger_schedule:
Delay some time then start a progress logger task.
Call `progress_logger_stop` to remove the task from the
queue before it starts or to cancel it while it's running.
*/
static void
progress_logger_schedule (MtxViewer *mvr)
{
    const guint id = g_timeout_add_seconds (2, G_SOURCE_FUNC
                                            (progress_logger_start), mvr);
    g_queue_push_tail (mvr->progress_logger_q, GUINT_TO_POINTER (id));
}

/**
progress_logger_update:
@data: owned.
*/
static gboolean
progress_logger_update (progress_logger_update_data *data)
{
    const gchar *msgid[] = {
    [MTX_CMM_PROGRESS_START]                 = Q_("progress|Parsing..."),
    [MTX_CMM_PROGRESS_SHEBANG]               = Q_("progress|Shebang."),
    [MTX_CMM_PROGRESS_LEGACY]                = Q_("progress|Legacy support."),
    [MTX_CMM_PROGRESS_HEADINGS]              = Q_("progress|Headings."),
    [MTX_CMM_PROGRESS_PARSED]                = Q_("progress|Parsed."),
    [MTX_CMM_PROGRESS_CONSOLIDATED]          = Q_("progress|Units optimized."),
    [MTX_CMM_PROGRESS_COLLAPSED]             = Q_("progress|Inlines optimized."),
    [MTX_CMM_PROGRESS_ELIDED]                = Q_("progress|Blocks optimized."),
    [MTX_CMM_PROGRESS_TABLE_PREPROCESSED]    = Q_("progress|Tables loaded."),
    [MTX_CMM_PROGRESS_TABLE_JUSTIFIED]       = Q_("progress|Tables formatted."),
    [MTX_CMM_PROGRESS_TEXT_TRANSFORMED]      = Q_("progress|Text filters applied."),
    [MTX_CMM_PROGRESS_JOINED]                = Q_("progress|Text ready."),
    [MTX_CMM_PROGRESS_TOC]                   = Q_("progress|Table of Contents."),
    [MTX_CMM_PROGRESS_END]                   = Q_("progress|Markup ready."),
    [MTX_TEXT_VIEW_PROGRESS_START]           = Q_("progress|Rendering page..."),
    [MTX_TEXT_VIEW_PROGRESS_MARKUP_INSERTED] = Q_("progress|Content loaded."),
    [MTX_TEXT_VIEW_PROGRESS_IMAGES_LINKS]    = Q_("progress|Links loaded."),
    [MTX_TEXT_VIEW_PROGRESS_INDENTED]        = Q_("progress|Lines indented."),
    [MTX_TEXT_VIEW_PROGRESS_RENDERED]        = Q_("progress|Page rendered."),
    [MTX_TEXT_VIEW_PROGRESS_END]             = Q_("progress|Done."),
    };
    const gulong id = (gulong) data->id;
    GtkProgressBar *bar = data->mvr->progress_bar;
    g_free (data);
    const gulong N = G_N_ELEMENTS (msgid);
    g_assert (id < N);
    const gchar *message = msgid[id];
    const gdouble fraction = (id + 1) * 1.0 / N;
    gtk_progress_bar_set_text (bar, message);
    gtk_progress_bar_set_fraction (bar, fraction);
    return FALSE;
}

/**
progress_logger_thread_cb:
Update the progress bar once.
*/
static void
progress_logger_thread_cb (GTask *task,
                           gpointer source_object __attribute__((unused)),
                           gpointer task_data,
                           GCancellable *cancellable)
{
    FILE *fp;
    gchar buf[16];
    progress_logger_data *data = task_data;

    /* Handle cancellation. */
    if (g_task_return_error_if_cancelled (task))
    {
        g_cancellable_reset (cancellable);
        return;
    }

    if ((fp = fdopen (data->mvr->progress_fd[0], "r")) == NULL)
    {
        g_task_return_int (task, -1);
    }
    while (!g_cancellable_is_cancelled (cancellable) &&
           fgets (buf, sizeof buf, fp))
    {
        progress_logger_update_data *udat =
        g_new0 (progress_logger_update_data, 1);
        udat->mvr = data->mvr;
        udat->id = atoi (buf);
        g_idle_add (G_SOURCE_FUNC (progress_logger_update), udat);
    }
    g_cancellable_reset (cancellable);
    g_task_return_int (task, 0);
}

/**
progress_logger_async:
Start a thread task that will keep udating the progress bar.
*/
static void
progress_logger_async (GObject *object __attribute__((unused)),
                       MtxViewer *mvr,
                       GCancellable *cancellable __attribute__((unused)),
                       GAsyncReadyCallback callback __attribute__((unused)),
                       gpointer user_data __attribute__((unused)))
{
    GTask *task = NULL;
    progress_logger_data *data;

    g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

    task = g_task_new (object, cancellable, callback, user_data);
    g_task_set_source_tag (task, progress_logger_async);

    if (g_task_set_return_on_cancel (task, TRUE))
    {

        data = g_new0 (progress_logger_data, 1);
        data->mvr = mvr;
        g_task_set_task_data (task, data, g_free);

        /* When it's done it will call @callback in
        the current thread default main context. */
        g_task_run_in_thread (task, progress_logger_thread_cb);
    }

    g_object_unref (task);
}

/**
progress_logger_finish:
*/
static gint
progress_logger_finish (GAsyncResult *result,
                        GError **error)
{
    g_return_val_if_fail (G_IS_TASK (result) &&
                          g_task_get_source_tag (G_TASK (result))
                          == progress_logger_async, -1);
    g_return_val_if_fail (error == NULL || *error == NULL, -1);

    return g_task_propagate_int (G_TASK (result), error);
}

/**
progress_logger_completed:
*/
static void
progress_logger_completed (GObject *object __attribute__((unused)),
                           GAsyncResult *result,
                           gpointer user_data __attribute__((unused)))
{
    g_assert (object == NULL); /* future expansion */
    g_assert (user_data == NULL);

    GError *error = NULL;
    gint ret __attribute__((unused)) = progress_logger_finish (result, &error);
    /* error->message can be "Operation was cancelled." */
    if (error != NULL)
    {
        g_error_free (error);
    }
}

static gboolean
statusbar_warn_pop (gpointer data)
{
    MtxViewer *mvr = data;
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign (gtk_statusbar_get_message_area
                           (mvr->status_bar), GTK_ALIGN_START);
#endif
    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_WARN);
    return G_SOURCE_REMOVE;
}

/**
mtx_viewer_statusbar_warn_seconds:
Write warning message to the status bar and clear it after a delay.
*/
static void
mtx_viewer_statusbar_warn_seconds (MtxViewer *mvr,
                                   const guint seconds,
                                   const gchar *message)
{
    gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_WARN, message);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign (gtk_statusbar_get_message_area
                           (mvr->status_bar), GTK_ALIGN_END);
#endif
    g_timeout_add_seconds (seconds, G_SOURCE_FUNC (statusbar_warn_pop), mvr);
}

/**
mtx_viewer_is_page_in_progress
Is a page being loaded? If so warn the user.
*/
static gboolean
mtx_viewer_is_page_in_progress (MtxViewer *mvr)
{
    if (mvr->progress_logger_q->length > 0 ||
        mvr->progress_fd[0] || mvr->progress_fd[1])
    {
        mtx_viewer_statusbar_warn_seconds (mvr, 2, _(
            "::: Action not allowed while the page is loading :::"));
        return TRUE;
    };
    return FALSE;
}
/*************************************************************************}}}
****************************************************************************/

/****************************************************************************
* COMPLETION CALLBACKS PASSED TO mtx_viewer_route_page TO UPDATE THE GUI {{{*
****************************************************************************/

struct _completer_data
{
    enum
    {
        NAV_FORE_CB,
        NAV_BACK_CB,
        NAV_HOME_CLICKED_CB,
        ON_LINK_CLICKED_CB,
        SEARCH_ENTRY_ACTIVATE_CB,
        DO_INSERT_PAGE_CB,
        PRESENT_PAGE_CB,
        ERROR_PAGE_CB,
    } completer;

    union {
        struct {
            MtxViewer *mvr;
            guint saved_curpos;
            gchar *page;             /* owned */
        } nav_fore_cb;

        struct {
            MtxViewer *mvr;
            guint saved_curpos;
            gchar *page;             /* owned */
        } nav_back_cb;

        struct {
            MtxViewer *mvr;
            gchar *page;             /* owned */
        } nav_home_clicked_cb;

        struct {
            MtxViewer *mvr;
            guint offset;
            gchar *page;             /* owned */
        } on_link_clicked_cb;

        struct {
            MtxViewer *mvr;
            guint offset;
            const gchar *scheme;
            gchar *uri;              /* owned */
        } search_entry_activate_cb;

        struct {
            MtxViewer *mvr;
            guint offset;
            gchar *page;             /* owned */
        } do_insert_page_cb;

        struct {
            MtxViewer *mvr;
            guint offset;
            gchar *page;             /* owned */
        } present_page_cb;

        struct {
            MtxViewer *mvr;
            gchar *page;             /* owned */
        } error_page_cb;
    } args;
} completer_data;

static void
nav_fore_cb (gpointer *instance __attribute__((unused)),
             gboolean cond,
             GError *error,    /*owned */
             gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == NAV_FORE_CB);
    MtxViewer *mvr = p->args.nav_fore_cb.mvr;
    guint saved_curpos = p->args.nav_fore_cb.saved_curpos;
    g_autofree gchar *page = p->args.nav_fore_cb.page;
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        _nav_trail_fore (mvr);
        mtx_viewer_write_toc_to_backing_file (mvr);
    }
    else
    {
        mvr->current_curpos = saved_curpos;
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

static void
nav_back_cb (gpointer *instance __attribute__((unused)),
             gboolean cond,
             GError *error,    /*owned */
             gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == NAV_BACK_CB);
    MtxViewer *mvr = p->args.nav_back_cb.mvr;
    guint saved_curpos = p->args.nav_back_cb.saved_curpos;
    g_autofree gchar *page = p->args.nav_back_cb.page;
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        _nav_trail_back (mvr);
        mtx_viewer_write_toc_to_backing_file (mvr);
    }
    else
    {
        mvr->current_curpos = saved_curpos;
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

static void
nav_home_clicked_cb (gpointer *instance __attribute__((unused)),
                     gboolean cond,
                     GError *error,    /*owned */
                     gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == NAV_HOME_CLICKED_CB);
    MtxViewer *mvr = p->args.nav_home_clicked_cb.mvr;
    g_autofree gchar *page = p->args.nav_home_clicked_cb.page;
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        _nav_trail_fore_clear (mvr);
        mvr->current_curpos = 0;
        mtx_viewer_write_toc_to_backing_file (mvr);
    }
    else
    {
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

static void
on_link_clicked_cb (gpointer *instance __attribute__((unused)),
                    gboolean cond,
                    GError *error,    /*owned */
                    gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == ON_LINK_CLICKED_CB);
    MtxViewer *mvr = p->args.on_link_clicked_cb.mvr;
    guint offset = p->args.on_link_clicked_cb.offset;
    gchar *page = p->args.on_link_clicked_cb.page;    /* owned */
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        _nav_trail_fore_clear (mvr);
        _nav_trail_insert (mvr, mvr->current_file, offset);
        mvr->current_curpos = 0;
        mtx_viewer_write_toc_to_backing_file (mvr);
    }
    else
    {
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    g_free (page);
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

static void
search_entry_activate_cb (gpointer *instance __attribute__((unused)),
                          gboolean cond,
                          GError *error,    /*owned */
                          gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == SEARCH_ENTRY_ACTIVATE_CB);
    MtxViewer *mvr = p->args.search_entry_activate_cb.mvr;
    guint offset = p->args.search_entry_activate_cb.offset;
    const gchar *scheme = p->args.search_entry_activate_cb.scheme;
    gchar *uri = p->args.search_entry_activate_cb.uri;    /* owned */
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        if (scheme == /*search*/NULL || strcmp (scheme, "search") == 0)
        {
            /*
            Unlike nav_home_clicked and on_link_clicked, which clear
            the fore trail on mtx_viewer_route_page success, in this
            case I prefer to insert the search results page in the trail
            after the current page, without clearing the fore trail.
            */
            _nav_trail_insert (mvr, uri, offset);
        }
        /* reminder: new schemes added below shall manage navigation history */
    }
    else
    {
        mtx_viewer_insert_error_page (mvr, uri, error);
    }
    g_free (uri);
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

/**
do_insert_page_cb:
Insert the page into the navigation trail after
the current slot without clearing the fore trail.
*/
static void
do_insert_page_cb (gpointer *instance __attribute__((unused)),
                   gboolean cond,
                   GError *error,    /*owned */
                   gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == DO_INSERT_PAGE_CB);
    MtxViewer *mvr = p->args.do_insert_page_cb.mvr;
    guint offset = p->args.do_insert_page_cb.offset;
    gchar *page = p->args.do_insert_page_cb.page;    /* owned */
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        gboolean tracked = mtx_viewer_is_current_tracked (mvr);
        if (tracked)
        {
            _nav_trail_insert (mvr, page, offset);
        }
        mtx_viewer_write_toc_to_backing_file (mvr);
        gtk_widget_set_sensitive (mvr->btn_preview, tracked);
    }
    else
    {
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    g_free (page);
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

static void
present_page_cb (gpointer *instance __attribute__((unused)),
                 gboolean cond,
                 GError *error,    /* owned */
                 gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == PRESENT_PAGE_CB);
    MtxViewer *mvr = p->args.present_page_cb.mvr;
    guint offset = p->args.present_page_cb.offset;
    gchar *page = p->args.present_page_cb.page;    /* owned */
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (cond)
    {
        gboolean tracked = mtx_viewer_is_current_tracked (mvr);
        if (tracked)
        {
            _nav_trail_fore_clear (mvr);
            _nav_trail_insert (mvr, mvr->current_file, offset);
        }
        mtx_viewer_write_toc_to_backing_file (mvr);
        gtk_widget_set_sensitive (mvr->btn_preview, tracked);
    }
    else
    {
        mtx_viewer_insert_error_page (mvr, page, error);
    }
    gtk_window_present (GTK_WINDOW (mvr->window));
    gtk_widget_grab_focus (GTK_WIDGET (mvr->text_view));
    g_free (page);
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
#ifdef OPT_EXIT_TEST
    if (mvr->exit_test)
    {
        g_idle_add ((GSourceFunc) gtk_main_quit, NULL);
    }
#endif
}

/**
error_page_cb:
Minimalistic handler for errors that may occur while presenting the error page.
*/
static void
error_page_cb (gpointer *instance __attribute__((unused)),
               gboolean cond __attribute__((unused)),
               GError *error,    /* owned */
               gpointer data[])
{
    struct _completer_data *p = (struct _completer_data *) data;
    g_assert (p->completer == ERROR_PAGE_CB);
    MtxViewer *mvr = p->args.error_page_cb.mvr;
    gchar *page = p->args.error_page_cb.page;    /* owned */
    g_autoptr (GError) err = error;

    progress_logger_stop (mvr);
    if (error != NULL)
    {
        g_prefix_error (&error, "\"%s\": ", page);
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_WARN,
                            error->message);
    }
    gtk_window_present (GTK_WINDOW (mvr->window));
    gtk_widget_grab_focus (GTK_WIDGET (mvr->text_view));
    g_free (page);
    g_free (data);
    mtx_viewer_widgets_set_sensitive (mvr, TRUE);
}

/**
mtx_viewer_write_toc_to_backing_file:
Helper for completer callbacks.
*/
static void
mtx_viewer_write_toc_to_backing_file (MtxViewer *mvr)
{
    const gchar *toc = mtx_text_view_fetch_page_toc_md (mvr->text_view);
    if (toc != NULL)
    {
        mtx_viewer_save_backing_file (mvr, toc, strlen (toc));
    }
}

/*************************************************************************}}}
****************************************************************************/

/**
mtx_viewer_insert_error_page:
Display the standard error page instead of a faulty/missing page.

@mvr:
@page: path of the faulty/missing page to fill the error page.
@error: pointer to %GError to fill the error page.
*/
static void
mtx_viewer_insert_error_page (MtxViewer *mvr,
                              const gchar *page,
                              GError *error)
{
    gchar *message, *mkd;
    const gchar *errmsg = (error != NULL ? error->message :
                           g_strerror (ENOENT));   /* just guessing */
    g_assert (mvr->failed_file == NULL);
    mvr->failed_file = g_strdup (page);
    message =
    g_strdup_printf (Q_ ("1=path:2=error|%1$s:\n%2$s."), page, errmsg);
    mkd = mtx_viewer_make_error_page (mvr, message, mvr->current_file != NULL
                                      ? mvr->current_file : USAGE_PAGE);
    g_free (message);
    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_MAIN);
    if (mkd != NULL)
    {
        struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
        cdat->completer = ERROR_PAGE_CB;
        cdat->args.error_page_cb.mvr = mvr;
        cdat->args.error_page_cb.page = g_strdup (page);
        GClosure *do_error_page_complete =
        g_cclosure_new (G_CALLBACK (error_page_cb), cdat, NULL);
        g_closure_set_marshal (do_error_page_complete,
                               g_cclosure_marshal_VOID__UINT_POINTER);

        gchar **pptr = g_malloc (sizeof (gchar *));
        *pptr = mkd;
        mtx_text_view_set_text (mvr->text_view, pptr, "/", TRUE, NULL,
                                do_error_page_complete);
    }
}

/**
mtx_viewer_route_page:
Asynchronously load a new page from a file path or a URI.
Supported URIs: `resource://...`, `search://...`.
on_link_clicked() handles URI schemes `http:`, `https:`, `ftp:` and `mailto:`.

Returns: TRUE if a new page was loaded otherwise returns FALSE.
*/
/*
Discipline for navigation history:
Viewer navigation history shall be managed only by the @completer closure
and by the `file_load_complete` callback, which is connected to the
"file-load-complete" signal emitted by `mtx_text_view_load_file()`.
*/
static gboolean
mtx_viewer_route_page (MtxViewer *mvr,
                       const gchar *path,
                       GClosure *completer)
{
    const gchar *scheme = g_uri_peek_scheme (path);
    gboolean retval = FALSE; /* => found an unknown scheme */

    mtx_viewer_widgets_set_sensitive (mvr, FALSE);
    g_clear_pointer (&mvr->failed_file, g_free);
    if G_UNLIKELY (g_strcmp0 (scheme, "search") == 0)
    {
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN,
                            _("Searching ..."));
        progress_logger_schedule (mvr);

        /* Partially async (searching itself isn't). */
        retval = mtx_viewer_search_files (mvr, path + sizeof ("search://") -
                                          1, path, completer);
    }
    else if (g_strcmp0 (scheme, "resource") == 0)
    {
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN,
                            _("Loading ..."));
        progress_logger_schedule (mvr);

        /* Partially async (resource unpacking isn't). */
        const gchar *p = path + sizeof "resource://" - 1;
        retval = mtx_viewer_load_resource (mvr, p, p, path, completer);
    }
    else if (scheme == NULL)
    {
        g_autofree gchar *p = g_path_get_basename (path);
        g_autofree gchar *m = g_strdup_printf (_("Loading %s ..."), p);
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN, m);
        progress_logger_schedule (mvr);

        /* Fully async. */
        retval =
        mtx_text_view_load_file (mvr->text_view, path, mvr->current_file ==
                                 NULL ? "" : mvr->current_file, TRUE,
                                 completer);
    }

    mtx_dbg_errout (1, "end %s\n", mtx_dbg_fmt_etime (-1));
    return retval;
}

/**
nav_fore_clicked:

Asynchronous page presentation task, calling `nav_fore_cb` to deal
with the result and show an error page if necessary.
*/
static void
nav_fore_clicked (GtkWidget *widget __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxViewerNavUnit *fore =
    (MtxViewerNavUnit *) g_queue_peek_nth (mvr->nav_trail,
                                           mvr->nav_trail_page_idx + 1);
    if (fore == NULL)
    {
        /* Bail out from chain of page navigation errors. */
        _nav_trail_fore_clear (mvr);
        return;
    }
    gchar *page = fore->file;
    guint saved_curpos = mvr->current_curpos;
    ((MtxViewerNavUnit *) mvr->nav_trail_page)->offset = mvr->changed_curpos;
    mvr->current_curpos = fore->offset;

    struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
    cdat->completer = NAV_FORE_CB;
    cdat->args.nav_fore_cb.mvr = mvr;
    cdat->args.nav_fore_cb.saved_curpos = saved_curpos;
    cdat->args.nav_fore_cb.page = g_strdup (page);
    GClosure *nav_fore_complete =
    g_cclosure_new (G_CALLBACK (nav_fore_cb), cdat, NULL);
    g_closure_set_marshal (nav_fore_complete,
                           g_cclosure_marshal_VOID__UINT_POINTER);

    (void) mtx_viewer_route_page (mvr, page, nav_fore_complete);
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
    if (mvr->can_go_fore && !mtx_viewer_is_page_in_progress (mvr))
    {
        nav_fore_clicked (NULL, mvr);
    }
    return TRUE;
}

/**
nav_back_clicked:

Asynchronous page presentation task, calling `nav_back_cb` to deal
with the result and show an error page if necessary.
*/
static void
nav_back_clicked (GtkWidget *widget __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    MtxViewerNavUnit *back =
    (MtxViewerNavUnit *) g_queue_peek_nth (mvr->nav_trail,
                                           mvr->nav_trail_page_idx - 1);
    gchar *page = back->file;
    guint saved_curpos = mvr->current_curpos;
    ((MtxViewerNavUnit *) mvr->nav_trail_page)->offset = mvr->changed_curpos;
    mvr->current_curpos = back->offset;

    struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
    cdat->completer = NAV_BACK_CB;
    cdat->args.nav_back_cb.mvr = mvr;
    cdat->args.nav_back_cb.saved_curpos = saved_curpos;
    cdat->args.nav_back_cb.page = g_strdup (page);
    GClosure *nav_back_complete =
    g_cclosure_new (G_CALLBACK (nav_back_cb), cdat, NULL);
    g_closure_set_marshal (nav_back_complete,
                           g_cclosure_marshal_VOID__UINT_POINTER);

    (void) mtx_viewer_route_page (mvr, page, nav_back_complete);
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
    if (mvr->can_go_back && !mtx_viewer_is_page_in_progress (mvr))
    {
        nav_back_clicked (NULL, mvr);
    }
    return TRUE;
}

/**
*/
static gboolean
link_info_dest_equal (gconstpointer *a,
                      gconstpointer *b)
{
    return g_strcmp0 (((MtxTextViewLinkInfo *) a)->dest,
                      ((MtxTextViewLinkInfo *) b)->dest) == 0;
}

/**
on_link_clicked:

Asynchronous page presentation task, calling `on_link_clicked_cb` to deal
with the result and show an error page if necessary. This function is
called from the #MtxTextView class.

@link_dest: format: <uri-encoded>\n<verbatim>

Note: if the name of a local file starts with "#" then the
destination of a markdown link to the file must start with "file://"
otherwise the destination will be processed as an in-page anchor.
*/
static void
on_link_clicked (MtxTextView *text_view,
                 const gchar *link_dest,
                 gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *nl = strchr (link_dest, '\n');       /* uri-encoded */
    g_assert (nl != NULL);
    const gchar *scheme = g_uri_peek_scheme (link_dest);

    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_LINK);

    if (scheme && (strcmp (scheme, "https") == 0 || strcmp (scheme, "http") == 0
             || strcmp (scheme, "ftp") == 0 || strcmp (scheme, "mailto") == 0))
    {
        gchar *uri = g_strndup (link_dest, nl - link_dest);
        open_url (mvr, uri);
        g_free (uri);
        return;
    }
    if (mtx_viewer_is_page_in_progress (mvr))
    {
        return;
    }

    /* Read the note in the top comment. */
    if (link_dest[0] == '#'
        && mvr->options->extensions & MTX_CMM_EXTENSION_HEADING_LINK)
    {
        /* Scroll to the matching heading. */

        if (link_dest[1])
        {
            guint index;
            MtxTextViewLinkInfo *p, link_info = {0};
            GtkTextIter iter;
            link_info.dest = g_strndup (link_dest, nl - link_dest);
            link_info.type = MTX_TEXT_VIEW_LINK_INFO_TYPE_HEADING;
            /* Does the link destination match a reference link or a slug? */
            gboolean found = g_ptr_array_find_with_equal_func
                (text_view->link_marks, &link_info,
                 (GEqualFunc) link_info_dest_equal, &index);
            if (!found)
            {
                /* No match; try lowercase because slugs are lowercase. */
                for (gchar *c = (gchar *) link_info.dest; *c; c++)
                {
                    *c = g_ascii_tolower (*c);
                }
                found = g_ptr_array_find_with_equal_func
                    (text_view->link_marks, &link_info,
                     (GEqualFunc) link_info_dest_equal, &index);
            }
            if (found)
            {
                if (mvr->landing_link_info)
                {
                    /* The previous landing spot. It's highlighted. */
                    mtx_text_view_clear_line_highlights (text_view,
                                                         mvr->
                                                         landing_link_info->
                                                         mark);
                }
                p = g_ptr_array_index (text_view->link_marks, index);
                gtk_text_buffer_get_iter_at_mark (text_view->buffer, &iter,
                                                  p->mark);
                mvr->current_curpos = gtk_text_iter_get_offset (&iter);
                /* The new landing spot. It could carry stale highlights. */
                mvr->landing_link_info = p;
                if (text_view->jumpoff_mark != NULL)
                {
                    mtx_text_view_clear_line_highlights (text_view,
                                                         text_view->
                                                         jumpoff_mark);
                }
                /* Now jump to the new spot. Since it's an in-page jump it
                   can be done immediately, without going through g_idle_add
                   like it happens for idle_scroll_to_current_curpos. */
                _scroll_to_curpos (mvr, mvr->current_curpos,
                                   MTX_TEXT_VIEW_HILIGHT_CLEAR_LINE |
                                   MTX_TEXT_VIEW_HILIGHT_NORMAL);
            }
            g_free ((gchar *) link_info.dest);
        }
    }
    else if (scheme == NULL || strcmp (scheme, "file") == 0 ||
             strcmp (scheme, "resource") == 0 || strcmp (scheme, "search") == 0)
    {
        const gchar *page = (g_strcmp0 (scheme, "file") == 0 ? nl + sizeof
                             "file://" : nl + 1);
        const guint offset = mvr->changed_curpos;

        struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
        cdat->completer = ON_LINK_CLICKED_CB;
        cdat->args.on_link_clicked_cb.mvr = mvr;
        cdat->args.on_link_clicked_cb.offset = offset;
        cdat->args.on_link_clicked_cb.page = g_strdup (page);
        GClosure *on_link_clicked_complete =
        g_cclosure_new (G_CALLBACK (on_link_clicked_cb), cdat, NULL);
        g_closure_set_marshal (on_link_clicked_complete,
                               g_cclosure_marshal_VOID__UINT_POINTER);

        (void) mtx_viewer_route_page (mvr, page, on_link_clicked_complete);
    }
}

/**
*/
static void
cancel_loading_clicked (GtkWidget *widget __attribute__((unused)),
                        gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    mtx_text_view_load_markup_cancel (mvr->text_view);
    gtk_widget_set_visible (mvr->progress_box, FALSE);
    gtk_statusbar_pop  (mvr->status_bar, STATUSBAR_CTX_MAIN);
    gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN,
                        _("Cancelling..."));
}

/**
*/
static gboolean
accel_cancel_loading (GtkAccelGroup *group __attribute__((unused)),
                      GObject *obj __attribute__((unused)),
                      guint keyval __attribute__((unused)),
                      GdkModifierType mod __attribute__((unused)),
                      gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    if (gtk_widget_get_visible (mvr->progress_box))
    {
        cancel_loading_clicked (NULL, mvr);
    }
    return TRUE;
}

/**
preview_complete:
Open the browser if mdview --html ran successfully.
*/
static void
preview_complete (GPid     pid,
                  gint     status,
                  gpointer user_data)
{
    struct {
        GtkWidget *btn;
        MtxViewer *mvr;
        gchar *uri;  /* owned */
    } *udat = user_data;

    g_spawn_close_pid (pid);
    if (g_spawn_check_wait_status (status, NULL))
    {
        open_url (udat->mvr, udat->uri);
    }
    gtk_widget_set_sensitive (udat->btn, TRUE);
    g_free (udat->uri);
    g_free (udat);
}

/**
preview_clicked:
Run mdview --html current_file asynchronously, and arrange for pick-up.
*/
static void
preview_clicked (GtkWidget *widget,
                 gpointer data)
{
    gchar **argv = NULL;
    GPid child_pid;
    g_autoptr (GError) error = NULL;
    MtxViewer *mvr = (MtxViewer *) data;
    if (widget == NULL)
    {
        widget = mvr->btn_preview;
    }
    gtk_widget_set_sensitive (widget, FALSE);

    const gchar *dir, *file;
    gchar *base;
    if G_LIKELY (g_strcmp0 (g_uri_peek_scheme (mvr->current_file), "search"))
    {
        dir = mvr->base_directory;
        file = mvr->current_file;
        base = g_path_get_basename (file);
    }
    else
    {
        dir = mvr->backing_file;
        file = "";
        base =
        g_strdelimit (g_strdup (mvr->current_file + sizeof "search://" - 1),
                      " \t" G_DIR_SEPARATOR_S, '_');
    }

    gchar *p = g_build_filename (g_get_tmp_dir (), base, NULL);
    g_autofree gchar *outf = g_strconcat (p, ".html", NULL);
    g_free (p);
    g_free (base);
    g_autofree gchar *html_base = NULL;

    if (mvr->options->html_base != NULL)
    {
        html_base = g_strconcat ("--html-base=", mvr->options->html_base, NULL);
    }
    p = g_strdup_printf (PROGNAME " --emask=%d" " --tmask=%d"
                         " --html --html5 --html-full" " %s --html-css=%d"
                         " --toc-level=%d" " \"--output=%s\"" " \"%s\""
                         " \"%s\"", mvr->options->extensions,
                         mvr->options->tweaks | MTX_CMM_TWEAK_RESERVED3,
                         html_base ? html_base : "",
                         mvr->options->html_css < 0 ? 2 :
                         mvr->options->html_css, mvr->options->toc_level,
                         outf, dir, file);
    if (g_shell_parse_argv (p, NULL, &argv, &error) && error == NULL)
    {
        g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_SEARCH_PATH |
                                  G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                                  NULL, &child_pid, NULL, NULL, NULL, &error);
    }
    g_strfreev (argv);
    if (error != NULL)
    {
        g_printerr (_("%s: Error: %s\n"), PROGNAME, error->message);
        gtk_widget_set_sensitive (widget, TRUE);
        return;
    }

    struct
    {
        GtkWidget *btn;
        MtxViewer *mvr;
        gchar *uri;
    } *udat = g_malloc (sizeof *udat);
    udat->btn = widget;
    udat->mvr = mvr;
    udat->uri = g_strconcat ("file://", outf, NULL);
    g_child_watch_add (child_pid, preview_complete, udat);
}

/**
*/
static gboolean
accel_preview (GtkAccelGroup *group __attribute__((unused)),
               GObject * obj __attribute__((unused)),
               guint keyval __attribute__((unused)),
               GdkModifierType mod __attribute__((unused)),
               gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    if (!mtx_viewer_is_page_in_progress (mvr))
    {
        preview_clicked (NULL, mvr);
    }
    return TRUE;
}

/**
on_toc_changed:

Asynchronous page presentation task, calling `do_insert_page_cb`
to deal with the result and show an error page if necessary.
This function changes ToC depth then reloads the current page.
*/
static void
on_toc_changed (GtkWidget *widget,
                gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *text =
    gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (widget));
    const guint level = text[0] - '0';
    g_free (text);

    if (mvr->current_file != NULL && mvr->options->toc_level != level)
    {
        MtxViewerNavUnit *rip = NULL;
        if (mvr->nav_trail->length > 0)
        {
            rip = g_queue_pop_nth (mvr->nav_trail, mvr->nav_trail_page_idx);
            g_assert (rip);
            /* _nav_trail_back can leave mvr->nav_trail_page temporarily NULL */
            _nav_trail_back (mvr);
        }

        mvr->options->toc_level = level;
        mtx_text_view_set_toc_level (mvr->text_view, level);

        struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
        cdat->completer = DO_INSERT_PAGE_CB;
        cdat->args.do_insert_page_cb.mvr = mvr;
        cdat->args.do_insert_page_cb.offset = 0;
        cdat->args.do_insert_page_cb.page =
        g_strdup (rip ? rip->file : mvr->current_file);
        GClosure *do_insert_page_complete =
        g_cclosure_new (G_CALLBACK (do_insert_page_cb), cdat, NULL);
        g_closure_set_marshal (do_insert_page_complete,
                               g_cclosure_marshal_VOID__UINT_POINTER);

        (void) mtx_viewer_route_page (mvr, cdat->args.do_insert_page_cb.page,
                                      do_insert_page_complete);
        if (rip != NULL)
        {
            _nav_unit_clear (rip, mvr);
        }
    }
}

/**
*/
static gboolean
accel_toc (GtkAccelGroup *group __attribute__((unused)),
           GObject * obj __attribute__((unused)),
           guint keyval __attribute__((unused)),
           GdkModifierType mod __attribute__((unused)),
           gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    if (!mtx_viewer_is_page_in_progress (mvr))
    {
        gtk_combo_box_popup (GTK_COMBO_BOX (mvr->combo_toc));
    }
    return TRUE;
}

/**
file_load_complete:
Callback from #MtxTextView class and, in some cases, called directly by
#mtx_viewer_route_page.
*/
static void
file_load_complete (MtxTextView *text_view,
                    const gchar *file,
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    const gchar *scheme = g_uri_peek_scheme (file);

    if (mtx_viewer_is_current_tracked (mvr))
    {
        gchar *message = NULL;

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
        gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_LINK);
        gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_WARN);
        gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_MAIN, message);
        g_free (message);
    }
    else
    {
        _nav_trail_print_status_bar (mvr);
    }

    mtx_text_view_clear_page_highlights (text_view);
    /* Do not set mvr->current_curpos here! */

    /* Set the currently-loaded file. */
    g_free (mvr->current_file);
    mvr->current_file = g_strdup (file);

    mvr->landing_link_info = NULL;

    progress_logger_stop (mvr);

    /* Scroll only after the new page has finished loading */
    g_idle_add (G_SOURCE_FUNC (idle_scroll_to_current_curpos), mvr);
}

/**
on_new_text_buffer:
Callback on signal by #MtxTextView mtx_text_view_swap_buffer.
*/
static void
on_new_text_buffer (MtxTextView *text_view,
                    gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    g_signal_connect (text_view->buffer, "notify::cursor-position", G_CALLBACK
                      (on_curpos_changed), mvr);
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
    gtk_statusbar_push (mvr->status_bar, STATUSBAR_CTX_LINK, temp);
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

    gtk_statusbar_pop (mvr->status_bar, STATUSBAR_CTX_LINK);
}

/**
mtx_viewer_is_current_tracked:
Does the current page meta data request not to add the page to the trail?
*/
static gboolean
mtx_viewer_is_current_tracked (MtxViewer *mvr)
{
    const MtxCmmPageMeta *meta =
    mtx_text_view_fetch_page_meta (mvr->text_view);
    gboolean ret = meta == NULL || meta->viewer_track_page;
    return ret;
}

/**
mtx_viewer_is_current_skipping_toc:
Does the current page meta data allow the Table of Contents feature?
*/
static gboolean
mtx_viewer_is_current_skipping_toc (MtxViewer *mvr)
{
    const MtxCmmPageMeta *meta =
    mtx_text_view_fetch_page_meta (mvr->text_view);
    gboolean ret = meta == NULL || meta->renderer_skip_toc;
    return ret;
}

static void
mtx_viewer_widgets_set_sensitive (MtxViewer *mvr,
                                  gboolean enable)
{
    gtk_widget_set_sensitive (mvr->btn_preview,
                              mtx_viewer_is_current_tracked (mvr));
    gtk_widget_set_sensitive (mvr->combo_toc,
                              !mtx_viewer_is_current_skipping_toc (mvr));
    gtk_widget_set_sensitive (mvr->top_bar, enable);
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
are considered. The list of searched files honors the auto_language extension
feature (an existing File.$LANG.ext is preferred over File.ext).

@markdown: address of a #GSList pointer that receives the list of
absolute pathnames of the matching markdown files in the directory.
@text: address of a #GSList pointer that receives the list of
absolute pathnames of the other matching text files in the directory.
@base_length: pointer to a return location holding the byte length of the
canonicalized homepage directory path including the trailing path separator.

Return: the total number of elements in the two lists or -1 in case of error.
List elements can be NULL. *@markdown and @text are NULL if a list is empty.
The caller owns the returned lists and should free them when done.
*/
static gint
_build_search_lists (MtxViewer *mvr,
                     GSList **markdown,
                     GSList **text,
                     gsize *base_length)
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
            if (mvr->options->extensions & MTX_CMM_EXTENSION_AUTO_LANG)
            {
                filename = mtx_text_view_auto_lang_find (mvr->text_view, path);
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
    *base_length = strlen (abs_dirpath);
    return counter;
}

/**
_file_search:
Search file for matching terms and append a Markdown link if the match is found.

@path: file to search.
@pod: pointer to private POD structure containing in/out parameters.

The link destination is a relative pathname.
*/
static void
_file_search (gpointer path,
              gpointer pod)
{
    typedef struct
    {
        gboolean is_text_markdown;
        GString *retstr;
        gchar **terms;
        guint *ctr;
        GRegex *regex_astx, *regex_emptiness;
        GtkEntry *entry;
        gsize base_offset;
    } POD;
    POD *ppod = (POD *) pod;
    const gboolean is_text_markdown = ppod->is_text_markdown;
    GString *retstr    = ppod->retstr;
    gchar **terms      = ppod->terms;
    guint *counter     = ppod->ctr;
    const GRegex *regex_astx = ppod->regex_astx;
    const GRegex *emptiness = ppod->regex_emptiness;
    GtkEntry *entry    = ppod->entry;
    gboolean found = FALSE;

    gtk_entry_progress_pulse (entry);
    errno = 0;
    g_autofree gchar *contents =
    mtx_text_view_mmap_read_file (path, NULL, TRUE);
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
        Extract the page title from the heading.
        */
        GString *title = NULL, *dest = NULL;
        g_autoptr (GMatchInfo) minfo = NULL;

        if (is_text_markdown
            && g_regex_match (regex_astx, contents, 0, &minfo))
        {
            g_autofree gchar *p = g_match_info_fetch_named (minfo, "TITLE");
            GError *err = NULL;
            if (p != NULL)
            {
                const gchar *t;
                g_autofree gchar *r =
                g_regex_replace_literal (emptiness, p, -1, 0, " ", 0, &err);
                for (t = r; *t == ' '; t++)
                    ;
                if (*t)
                {
                    title = g_string_new (t);
                }
            }
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
        title->str = g_strstrip (g_strdelimit (title->str, "\\\n\r", ' '));
        g_string_set_size (title, strlen (title->str));
        g_string_replace (title, "[", "\\[", -1);
        g_string_replace (title, "]", "\\]", -1);
        /* destination path relative to the homepage directory */
        dest = g_string_new (path + ppod->base_offset);
        g_string_replace (dest, ")", "\\)", -1);

        g_string_append_printf (retstr, "* [%s](%s)\n",
                                title->str, dest->str);
        g_string_free (title, TRUE);
        g_string_free (dest, TRUE);
    }
}

/**
mtx_viewer_search_files:
Load the results of a search URI into a new page.

@mvr: %MtxViewer instance.
@text: needle string.
@file_complete: string, file for which to call
file_load_complete (as if the "file-load-complete" signal
was emitted) if async completion is successful.
@completer: GClosure invoked after the asynchronous operations have completed.

Returns: TRUE if the new page was generated otherwise returns FALSE.
*/
/*
Result is a synthetic page.
We must not call mtx_text_view_load_file!
*/
static gboolean
mtx_viewer_search_files (MtxViewer *mvr,
                         const gchar *text,
                         const gchar *file_complete,
                         GClosure* completer)
{
    g_return_val_if_fail (file_complete != NULL, FALSE);

    GString *markdown = g_string_new (NULL);
    gchar *stripped, **terms;
    gint ctr_subjects, ctr_results = 0;
    gsize base_offset;
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
    ctr_subjects = _build_search_lists (mvr, &mkd, &txt, &base_offset);
    if (ctr_subjects < 0)
    {
        return FALSE;
    }
    base_offset += sizeof G_DIR_SEPARATOR_S - 1;

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
        const GRegex *regex_astx, *regex_emptiness;
        GtkEntry *entry;
        gsize base_offset;
    } POD;
    POD pod = { TRUE, markdown, terms, &ctr_results,
        mtx_text_view_get_regex_astx (mvr->text_view),
        mvr->regex_emptiness, entry, base_offset };

    if (mvr->regex_emptiness == NULL)
    {
        GError *err = NULL;
        pod.regex_emptiness = mvr->regex_emptiness =
        g_regex_new ("[\\p{Zs}\\p{Zp}\\p{Zl}\\v]+", 0, 0, &err);
        if (err != NULL)
        {
            g_error ("uni_separator regex: %s", err->message);
            g_error_free (err);
            return FALSE;
        }
    }

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
        mtx_viewer_save_backing_file (mvr, markdown->str, markdown->len);
        g_string_prepend (markdown, "<mtx><renderer><skip_toc>1</skip_toc></renderer></mtx>\n");
    }

    /* Display results. */
    gchar **pptr = g_malloc (sizeof (gchar *));
    *pptr = markdown->str;
    gboolean retval =
    mtx_text_view_set_text (mvr->text_view, pptr, NULL, TRUE, file_complete,
                            completer);
    g_string_free (markdown, FALSE); /* mtx_text_view_load_markup_data_free */

    gtk_entry_set_progress_fraction (entry, 0.0f);
    gtk_widget_set_sensitive (mvr->window, TRUE);
    g_strfreev (terms);
    return retval;
}

/**
mtx_viewer_load_resource:
Load the results of a resource URI into a new viewing page.

@mvr: The #MtxTextView instance.
@path: A disk file path. NULLABLE
@embed: Path of an embedded resource file. NULLABLE
@file_complete: string, file for which to call
file_load_complete (as if the "file-load-complete" signal
was emitted) if async completion is successful.
@completer: GClosure invoked after the asynchronous operations have completed.

Look for "PROGNAME/@path" in $XDG_USER_DATA:$XDG_DATA_DIRS;
if not found then use the embedded @uri.

Returns: TRUE if the new page was generated otherwise it returns FALSE.
*/
static gboolean
mtx_viewer_load_resource (MtxViewer *mvr,
                          const gchar *path,
                          const gchar *embed,
                          const gchar *file_complete,
                          GClosure *completer)
{
    g_return_val_if_fail (file_complete != NULL, FALSE);

    gboolean retval = FALSE;

    /* Possibly load a disk file. */
    if (path != NULL)
    {
        g_autofree gchar *file = NULL;
        gchar *contents = NULL; /* mtx_text_view_load_markup_data_free */

        for (const gchar * const *p = mvr->data_dirs; *p; p++)
        {

            file = g_build_filename (*p, PROGNAME, path, NULL);
            contents =
            mtx_text_view_get_file_contents (mvr->text_view, file, NULL, TRUE);
            if (contents != NULL)
            {
                gchar **pptr = g_malloc (sizeof (gchar *));
                *pptr = contents;
                retval = mtx_text_view_set_text (mvr->text_view, pptr, file,
                                                 TRUE, file_complete,
                                                 completer);
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
        const gchar *contents = bytes == NULL ? NULL :
            (const gchar *) g_bytes_get_data (bytes, NULL);
        /* Carry on regardless, letting errors bubble up to @completer. */
        if (TRUE)
        {
            gchar **pptr = g_malloc (sizeof (gchar *));
            *pptr = g_strdup (contents);
            if (contents != NULL)
            {
                mtx_viewer_save_backing_file (mvr, contents, strlen (contents));
            }
            retval = mtx_text_view_set_text (mvr->text_view, pptr, "/", TRUE,
                                             file_complete, completer);
        }
    }
    return retval;
}

/**
do_open_welcome_page:

Asynchronous page presentation task, calling `do_insert_page_cb` to deal
with the result and show an error page if necessary. This function loads
the welcome page.

@mvr: The #MtxTextView instance.
*/
static void
do_open_welcome_page (MtxViewer *mvr)
{
    if (mtx_viewer_is_page_in_progress (mvr))
    {
        return;
    }
    const gchar *page = WELCOME_PAGE;
    const guint offset = mvr->current_curpos = 0;
    /*
    For consistency with search_entry_activate_cb, I prefer not
    to clear the fore trail before inserting the resource:// URI.
    */
    struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
    cdat->completer = DO_INSERT_PAGE_CB;
    cdat->args.do_insert_page_cb.mvr = mvr;
    cdat->args.do_insert_page_cb.offset = offset;
    cdat->args.do_insert_page_cb.page = g_strdup (page);
    GClosure *do_insert_page_complete =
    g_cclosure_new (G_CALLBACK (do_insert_page_cb), cdat, NULL);
    g_closure_set_marshal (do_insert_page_complete,
                           g_cclosure_marshal_VOID__UINT_POINTER);

    (void) mtx_viewer_route_page (mvr, page, do_insert_page_complete);
}

/**
search_entry_activate:
Route the search entry text, setting focus to the search entry. It's an
asynchronous page presentation task, calling `search_entry_activate_cb`
to deal with the result and show an error page if necessary.
*/
static void
search_entry_activate (GtkEntry *entry, gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    const gchar *needle = gtk_entry_get_text (entry);

    if (mtx_viewer_is_page_in_progress (mvr))
    {
        return;
    }
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

        struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
        cdat->completer = SEARCH_ENTRY_ACTIVATE_CB;
        cdat->args.search_entry_activate_cb.mvr = mvr;
        cdat->args.search_entry_activate_cb.offset = offset;
        cdat->args.search_entry_activate_cb.scheme = scheme;
        cdat->args.search_entry_activate_cb.uri = g_strdup (uri);
        GClosure *search_entry_activate_complete =
        g_cclosure_new (G_CALLBACK (search_entry_activate_cb), cdat, NULL);
        g_closure_set_marshal (search_entry_activate_complete,
                               g_cclosure_marshal_VOID__UINT_POINTER);

        (void) mtx_viewer_route_page (mvr, uri, search_entry_activate_complete);
    }
    else
    {
        /* takes care of managing the navigation history */
        do_open_welcome_page (mvr);
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
            (void) mtx_text_view_find_text (mvr->text_view, needle, options);
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
nav_home_clicked:
Asynchronous page presentation task, calling `nav_home_clicked_cb`
to deal with the result and show an error page if necessary. This
function resets page navigation history then reloads the home page.
*/
static void
nav_home_clicked (GtkWidget *button __attribute__((unused)),
                  gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;
    gchar *page = mvr->homepage == NULL ? DEFAULT_INDEX : mvr->homepage;

    while (mvr->nav_trail_page_idx > 0)
    {
        _nav_trail_back (mvr);
    }

    struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
    cdat->completer = NAV_HOME_CLICKED_CB;
    cdat->args.nav_home_clicked_cb.mvr = mvr;
    cdat->args.nav_home_clicked_cb.page = g_strdup (page);
    GClosure *nav_home_clicked_complete =
    g_cclosure_new (G_CALLBACK (nav_home_clicked_cb), cdat, NULL);
    g_closure_set_marshal (nav_home_clicked_complete,
                           g_cclosure_marshal_VOID__UINT_POINTER);

    (void) mtx_viewer_route_page (mvr, page, nav_home_clicked_complete);
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
    MtxViewer *mvr = (MtxViewer *) data;
    if (!mtx_viewer_is_page_in_progress (mvr))
    {
        nav_home_clicked (NULL, mvr);
    }
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
    MtxTextView *tv = mvr->text_view;
    gtk_text_buffer_get_iter_at_mark (tv->buffer, &iter, link_info->mark);
    gtk_text_buffer_place_cursor (tv->buffer, &iter);
    mtx_text_view_clear_page_highlights (tv);
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
    MtxTextView *tv = mvr->text_view;
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
    MtxTextView *tv = mvr->text_view;
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
    MtxViewer *mvr = (MtxViewer *) data;
    if (!mtx_viewer_is_page_in_progress (mvr))
    {
        (void) do_open_welcome_page (mvr);
    }
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
    const gchar *file_name = NULL;
    const gchar *scheme = g_uri_peek_scheme (mvr->current_file);

    if (g_strcmp0 (scheme, "search") == 0 || g_strcmp0 (scheme, "resource")== 0)
    {
        file_name = mvr->backing_file;
    }
    else
    {
        file_name =
        mvr->failed_file != NULL ? mvr->failed_file : mvr->current_file;
    }
    if (file_name != NULL)
    {
        gchar *path = NULL;
        if (file_name[0] != '/')
        {
            path = g_build_filename (tv->image_directory, file_name, NULL);
            if (!g_file_test (path, G_FILE_TEST_EXISTS))
            {
                gchar *name = g_path_get_basename (file_name);
                g_free (path);
                path = g_build_filename (tv->image_directory, name, NULL);
                g_free (name);
            }
            file_name = path;
        }
        edit_text_file (mvr, file_name);
        g_free (path);
    }
    return TRUE;
}

/**
mtx_viewer_make_error_page:
Return markdown error page with a custom message inside .

@mvr: pointer to #MtxViewer.
@message: string, often a printf format string.
@back_link: back-link return page. NULLABLE. If NULL a back-link is not added
to the page.

Returns: markdown string or NULL on error. The caller of this function owns
the returned memory.
*/
static gchar *
mtx_viewer_make_error_page (MtxViewer *mvr __attribute__((unused)),
                            const gchar *message,
                            const gchar *back_link)
{
    /* *INDENT-OFF* */
    g_autofree gchar *uri = NULL;
    g_autofree gchar *link = NULL;
    if (back_link == NULL)
    {
        link = g_strdup ("");
    }
    else
    {
        uri = g_markup_escape_text (back_link, -1);
        link = g_strdup_printf("[%s](%s)\n",
                               Q_("error page back-link label|Back"), uri);
    }
    return g_strdup_printf (
        "<mtx>"
            "<renderer>"
                "<keep_tags>1</keep_tags>"
                "<skip_toc>1</skip_toc>"
            "</renderer>"
            "<viewer>"
                "<track_page>0</track_page>"
            "</viewer>"
        "</mtx>\n"
        "<span size=\"x-large\">"
        /* Ensure an empty line after </span> to satisfy the end
        condition #6 of https://spec.commonmark.org/0.31.2/#html-block */
        "\n\n~~~~\n%s\n~~~~\n\n%s</span>\n\n",
        message, link);
    /* *INDENT-ON* */
}

/**
mtx_viewer_present_page:
Present a file or supported URI truncating the forward navigation trail.

@mvr: pointer to #MtxViewer.
@page: filepath or supported URI.
@offset: text buffer offset.

Return: TRUE if the page was successfully routed for presentation in the
viewer, otherwise FALSE. When the asynchronous routing task has finished,
the `present_page_cb` callback is invoked, which will deal with the result
by either refreshing the GUI or showing the error page.
*/
gboolean
mtx_viewer_present_page (MtxViewer *mvr,
                         const gchar *page,
                         guint offset)
{
    mtx_dbg_errout (1, "init timer %s\n",
                    mtx_dbg_fmt_etime (mtx_dbg_etime (0)));
    struct _completer_data *cdat = g_new0 (struct _completer_data, 1);
    cdat->completer = PRESENT_PAGE_CB;
    cdat->args.present_page_cb.mvr = mvr;
    cdat->args.present_page_cb.offset = offset;
    cdat->args.present_page_cb.page = g_strdup (page);
    GClosure *present_page_complete =
    g_cclosure_new (G_CALLBACK (present_page_cb), cdat, NULL);
    g_closure_set_marshal (present_page_complete,
                           g_cclosure_marshal_VOID__UINT_POINTER);
    return mtx_viewer_route_page (mvr, page, present_page_complete);
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
    if (mtx_viewer_is_page_in_progress (mvr))
    {
        accel_cancel_loading (NULL, NULL, 0, 0, mvr);
    }
    if (mvr->nav_trail != NULL)
    {
        if (mvr->nav_trail->length)
        {
            g_queue_foreach (mvr->nav_trail, (GFunc) _nav_unit_clear, mvr);
        }
        g_queue_clear (mvr->nav_trail);
        g_queue_free (mvr->nav_trail);
        mvr->nav_trail = NULL;
    }
    if (mvr->progress_logger_cancellable != NULL)
    {
        g_object_unref (mvr->progress_logger_cancellable);
    }
    if (mvr->progress_logger_q != NULL)
    {
        g_queue_free (mvr->progress_logger_q);
    }
    if (mvr->regex_emptiness != NULL)
    {
        g_regex_unref (mvr->regex_emptiness);
    }
    if (mvr->backing_fd >= 0)
    {
        close (mvr->backing_fd);
        unlink (mvr->backing_file);
        g_free (mvr->backing_file);
    }
    g_free (mvr->current_file);
    g_free (mvr->base_directory);
    g_free ((gpointer) mvr->data_dirs);
    if (mvr->parent == NULL && gtk_main_level ())
    {
        gtk_main_quit ();
    }
}

/**
*/
static gboolean
viewer_destroy_me (GtkWidget *widget __attribute__((unused)),
                   GdkEvent *event __attribute__((unused)),
                   gpointer data)
{
    MtxViewer *mvr = (MtxViewer *) data;

    mtx_viewer_destroy (mvr); /* quits gtk main */
    gtk_widget_destroy (widget);
    return TRUE;
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
            mtx_viewer_destroy (mvr);   /* quits gtk main */
            gtk_widget_destroy (widget);
            return TRUE;
    }
    return FALSE;
}

/**
mtx_viewer_tool_button_new_from_resource:
Make a toolbar button icon from an embedded resource at a given GTK button size.

@resource: embedded resource path.
@size_enum: #GtkButton size enumeration value.

Return: the icon widget or NULL on error.
*/
static GtkWidget *
mtx_viewer_button_icon_new_from_resource (const gchar *resource,
                                          const gint size_enum)
{
    gint w,h;

    gtk_icon_size_lookup (size_enum, &w, &h);
    GdkPixbuf *pb = gdk_pixbuf_new_from_resource (resource, NULL);
    GdkPixbuf *sp = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
    GtkWidget *im = gtk_image_new_from_pixbuf (sp);
    g_object_unref (pb);
    g_object_unref (sp);
    return im;
}

/**
mtx_viewer_new:

@base_dir: home page directory.
@base_file: home page.
@title: window title.
@parent: window.
@extensions: #MtxCmmExtensions flags.
@toc_level: table of contents maximum level.
@tweaks: #MtxCmmTweaks flags.
*/
MtxViewer *
mtx_viewer_new (const gchar *base_dir,
                const gchar *base_file,
                const gchar *title,
                GtkWindow *parent,
                const MtxViewerOptions *options)
{
    MtxViewer *mvr;
    GtkWidget *mtx_viewer;
    GtkWidget *vbox;
    GtkWidget *top_bar;
    GtkWidget *toolbar1;
    GtkWidget *separatortoolitem1;
    GtkWidget *toolbar2;
    GtkWidget *toolitem3;
    GtkWidget *toolitem4;
    GtkWidget *search_entry;
    GtkWidget *scrolled_mtx_viewer;
    MtxTextView *text_view;
    GtkWidget *progress_bar, *progress_box, *btn_cancel_loading;
    GtkWidget *status_bar;
    GtkWidget *btn_nav_back, *btn_nav_fore, *btn_nav_home, *btn_preview;
    GtkWidget *combo_toc;
    GtkAccelGroup *accel;

    mtx_viewer = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (mtx_viewer, PROGNAME"-main-win");
    gtk_widget_set_size_request (mtx_viewer, 300, 200);
    gtk_window_set_default_size (GTK_WINDOW (mtx_viewer), 640, 480);
    gtk_window_set_title (GTK_WINDOW (mtx_viewer),
                          title ? title : DEFAULT_WINDOW_TITLE);
    gtk_window_set_transient_for (GTK_WINDOW (mtx_viewer), parent);

    GdkPixbuf *icon = gdk_pixbuf_new_from_resource ("/app.svg", NULL);
    gtk_window_set_icon (GTK_WINDOW (mtx_viewer), icon);
    g_object_unref (icon);
#if !GTK_CHECK_VERSION(3,0,0)
    vbox = gtk_vbox_new (FALSE, 0);
#else
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (mtx_viewer), vbox);

#if !GTK_CHECK_VERSION(3,0,0)
    top_bar = gtk_hbox_new (FALSE, 0);
#else
    top_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_set_name (top_bar, PROGNAME"-top-bar");
    gtk_widget_show (top_bar);
    gtk_box_pack_start (GTK_BOX (vbox), top_bar, FALSE, FALSE, 0);

    toolbar1 = gtk_toolbar_new ();
    gtk_widget_show (toolbar1);
    gtk_box_pack_start (GTK_BOX (top_bar), toolbar1, TRUE, TRUE, 0);
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
                                 _("(Alt-B) Navigate back to the previous page"));
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
    gtk_widget_set_tooltip_text (btn_nav_fore,
                                 _("(Alt-F) Navigate forward to the next page"));
    gtk_widget_show (btn_nav_fore);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_nav_fore);
    /* gtk_tool_item_set_is_important (GTK_TOOL_ITEM (btn_nav_fore), TRUE); */
    gtk_widget_set_sensitive (btn_nav_fore, FALSE);

    GtkWidget *ico_preview =
    mtx_viewer_button_icon_new_from_resource ("/preview.svg",
                                              GTK_ICON_SIZE_LARGE_TOOLBAR);
    btn_preview = (GtkWidget *) gtk_tool_button_new (ico_preview,
                                                     _("HTML Preview"));
    gtk_widget_set_tooltip_text (btn_preview,
                                 _("(Alt-P) Preview the page in the browser"));
    gtk_widget_show (btn_preview);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_preview);

    combo_toc = gtk_combo_box_text_new ();
    gtk_widget_set_tooltip_text (combo_toc,
                                 _("(Alt-T) Change Table of Contents depth"));
    gtk_widget_show (combo_toc);
    GtkWidget *toolitem_toc = (GtkWidget *) gtk_tool_item_new ();
    gtk_widget_show (toolitem_toc);
    gtk_container_add (GTK_CONTAINER (toolbar1), toolitem_toc);
    gtk_container_add (GTK_CONTAINER (toolitem_toc), combo_toc);

    separatortoolitem1 = (GtkWidget *) gtk_separator_tool_item_new ();
    gtk_widget_show (separatortoolitem1);
    gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem1);

    toolbar2 = gtk_toolbar_new ();
    gtk_widget_show (toolbar2);
    gtk_box_pack_end (GTK_BOX (top_bar), toolbar2, FALSE, TRUE, 0);
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

    text_view = MTX_TEXT_VIEW (mtx_text_view_new ());
    mtx_text_view_set_extensions (text_view, options->extensions);
    mtx_text_view_set_toc_level (text_view, options->toc_level);
    mtx_text_view_set_tweaks (text_view, options->tweaks);
    mtx_text_view_set_image_directory (text_view, base_dir);
    mtx_text_view_set_auto_lang_find (text_view,
                                      options-> extensions &
                                       MTX_CMM_EXTENSION_AUTO_LANG);
    gtk_container_add (GTK_CONTAINER (scrolled_mtx_viewer),
                       GTK_WIDGET (text_view));

    /**********************************************************************
    *                            Progress Box                             *
    ******************************************************************{{{*/
#if !GTK_CHECK_VERSION(3,0,0)
    progress_box = gtk_hbox_new (FALSE, 0);
#else
    progress_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
    gtk_widget_set_name (progress_box, PROGNAME"-progress-box");
    gtk_widget_show (progress_box);

#if !GTK_CHECK_VERSION(3,0,0)
    btn_cancel_loading =
    (GtkWidget *) gtk_tool_button_new_from_stock ("gtk-cancel");
#else
    GtkWidget *icon_cancel =
    gtk_image_new_from_icon_name ("gtk-cancel", GTK_ICON_SIZE_LARGE_TOOLBAR);
    btn_cancel_loading =
    (GtkWidget *) gtk_tool_button_new (icon_cancel, _("Cancel page loading"));
#endif
    gtk_widget_set_tooltip_text (btn_cancel_loading,
                                 _("(Alt-X) Cancel loading this page"));
    gtk_box_pack_start (GTK_BOX (progress_box), btn_cancel_loading, FALSE,
                        FALSE, 0);
    gtk_widget_show (btn_cancel_loading);

    progress_bar = gtk_progress_bar_new ();
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_margin_end (progress_bar, 20);
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress_bar), TRUE);
    gtk_box_pack_start (GTK_BOX (progress_box), progress_bar, TRUE, TRUE, 0);
#else
    gtk_box_pack_start (GTK_BOX (progress_box), progress_bar, TRUE, TRUE, 0);
    GtkWidget *spacer = gtk_label_new (NULL);
    gtk_widget_set_size_request (spacer, 20, -1);
    gtk_box_pack_end (GTK_BOX (progress_box), spacer, FALSE, FALSE, 0);
#endif
    gtk_box_pack_start (GTK_BOX (vbox), progress_box, FALSE, FALSE, 0);
    /*******************************************************************}}}
    **********************************************************************/

    status_bar = gtk_statusbar_new ();
    gtk_widget_show (status_bar);
    gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);

    mvr = g_new0 (MtxViewer, 1);
    mvr->window = mtx_viewer;
    mvr->top_bar = top_bar;
    mvr->status_bar = GTK_STATUSBAR (status_bar);
    mvr->btn_nav_back = btn_nav_back;
    mvr->btn_nav_fore = btn_nav_fore;
    mvr->btn_preview = btn_preview;
    mvr->combo_toc = combo_toc;
    mvr->text_view = text_view;
    mvr->text_search = search_entry;
    mvr->backing_fd = g_file_open_tmp (PROGNAME "_backing_XXXXXX.md",
                                       &mvr->backing_file, NULL);
    mvr->progress_bar = GTK_PROGRESS_BAR (progress_bar);
    mvr->progress_box = progress_box;
    mvr->progress_logger_cancellable = g_cancellable_new ();
    mvr->progress_logger_q = g_queue_new ();
    mvr->base_directory = g_strdup (base_dir ? base_dir : ".");
    mvr->nav_trail = g_queue_new ();
    mvr->nav_trail_page = NULL;
    mvr->nav_trail_page_idx = -1;
    mvr->parent = GTK_WIDGET (parent);
    mvr->can_go_back = mvr->can_go_fore = FALSE;
    mvr->options = (MtxViewerOptions *) options;
#ifdef OPT_EXIT_TEST
    mvr->exit_test = options->tweaks & MTX_CMM_TWEAK_RESERVED2;
#endif

    g_signal_connect (mtx_viewer, "delete-event",
                      G_CALLBACK (viewer_destroy_me), mvr);
    g_signal_connect (mtx_viewer, "key-press-event",
                      G_CALLBACK (viewer_key_pressed), mvr);
    g_signal_connect (text_view, "link-clicked",
                      G_CALLBACK (on_link_clicked), mvr);
    g_signal_connect (text_view, "hovering-over-link",
                      G_CALLBACK (hovering_over_link), mvr);
    g_signal_connect (text_view, "hovering-over-text",
                      G_CALLBACK (hovering_over_text), mvr);
    g_signal_connect (text_view, "file-load-complete",
                      G_CALLBACK (file_load_complete), mvr);
    g_signal_connect (text_view, "new-text-buffer",
                      G_CALLBACK (on_new_text_buffer), mvr);
    on_new_text_buffer (text_view, mvr);
    g_signal_connect (btn_nav_back, "clicked", G_CALLBACK (nav_back_clicked),
                      mvr);
    g_signal_connect (btn_nav_fore, "clicked", G_CALLBACK (nav_fore_clicked),
                      mvr);
    g_signal_connect (btn_nav_home, "clicked", G_CALLBACK (nav_home_clicked),
                      mvr);
    g_signal_connect (btn_preview, "clicked", G_CALLBACK (preview_clicked),
                      mvr);
    g_signal_connect (combo_toc, "changed", G_CALLBACK (on_toc_changed), mvr);
    g_signal_connect (search_entry, "activate",
                      G_CALLBACK (search_entry_activate), mvr);
    g_signal_connect (search_entry, "icon-press",
                      G_CALLBACK (search_entry_icon_press), mvr);
    g_signal_connect (btn_cancel_loading, "clicked", G_CALLBACK
                      (cancel_loading_clicked), mvr);

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
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("p"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_preview),
                                             mvr, NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("t"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_toc),
                                             mvr, NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("e"),
                             GDK_CONTROL_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_edit_current),
                                             mvr, NULL));
    gtk_accel_group_connect (accel, gdk_keyval_from_name ("x"),
                             GDK_MOD1_MASK, 0,
                             g_cclosure_new (G_CALLBACK (accel_cancel_loading),
                                             mvr, NULL));
    gtk_window_add_accel_group (GTK_WINDOW (mtx_viewer), accel);

    /* build data search path */
    {
        gchar **a, **p, *u;
        /* Glib owns a and u */
        a = (gchar **) g_get_system_data_dirs ();
        u = (gchar *) g_get_user_data_dir ();
        p = g_new (gchar *, g_strv_length (a) + 1 + (*u ? 1 : 0));
        mvr->data_dirs = (const gchar * const *) p;
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

    mvr->homepage = (gchar *) (base_file == NULL ? DEFAULT_INDEX : base_file);
    if (base_dir == NULL)
    {
        base_dir = "";
    }
    if (base_dir[0] == '\0')
    {
        if (base_file == NULL)
        {
            base_file = USAGE_PAGE;
        }
    }
    g_autofree gchar *page = g_build_filename (base_dir, base_file, NULL);
    if (!g_file_test (page, G_FILE_TEST_EXISTS))
    {
        const gchar *scheme = g_uri_peek_scheme (base_file);
        if (scheme != NULL && (strcmp (scheme, "resource") == 0 || strcmp
                               (scheme, "search") == 0))
        {
            g_free (page);
            page = g_strdup (base_file);
        }
    }
    if (g_str_has_prefix (base_file, "resource://"))
    {
        mtx_text_view_set_image_directory (text_view, ".");
    }

    gtk_widget_show_all (mvr->window);
    gtk_widget_set_visible (progress_box, FALSE);
    {
        gchar b[2] = {0};
        for (b[0] = '6'; b[0] >= '0'; b[0]--)
        {
            gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (combo_toc), b);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_toc),
                                  mvr->options->toc_level);
    }
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }

    if (!mtx_viewer_present_page (mvr, page, 0)) /* can be the USAGE_PAGE */
    {
        /*
        Examples of generic error: ???
        */
        g_printerr (_("%s: generic error.\n"), PROGNAME);
        mtx_viewer_destroy (mvr);   /* quits gtk main */
        gtk_widget_destroy (mvr->window);
        g_free (mvr);
        return NULL;
    }

    return mvr;
}

