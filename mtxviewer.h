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
/**
2024-08-15 step:
This file was derived from the mdview3 help-viewer.h with substantial changes
and feature additions for MTX. The mdview3 help-viewer.h itself was derived
from the hardinfo "help-viewer" directory.
*/
/*
 *    HelpViewer - Simple Help file browser
 *    Copyright (C) 2009 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2015 James B
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

#ifndef __MTX_VIEWER_H__
#define __MTX_VIEWER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _MtxViewer MtxViewer;
struct _MtxViewer
{
    GtkWidget *parent;
    gchar *homepage;

    /*< private >**********************************************************/

    GtkWidget *window;
    GtkWidget *status_bar;

    GtkWidget *btn_nav_back, *btn_nav_fore;
    GtkWidget *text_view;
    GtkWidget *text_search;

    gboolean auto_lang;
    gchar *current_file;             /* the page about to be displayed */
    gint  current_curpos;
    gint  changed_curpos;
    gchar *base_directory;
    const gchar * const *data_dirs;

    GQueue *nav_trail;
    gpointer *nav_trail_page;        /* the page being displayed */
    gint nav_trail_page_idx;
    gboolean can_go_fore, can_go_back;

    GRegex *regex_astx;
};

MtxViewer *mtx_viewer_new (const gchar *, const gchar *, const gchar *, GtkWindow *, guint, guint);
gboolean mtx_viewer_present_page (MtxViewer *mtx_viewer, const gchar *, guint);
void mtx_viewer_destroy (MtxViewer *mtx_viewer);

G_END_DECLS

#endif /* __MTX_VIEWER_H__ */

