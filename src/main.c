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

#include <glib/gi18n.h>        /* xgettext --keyword=_ --keyword=Q_:1g */
#include <locale.h>

#include "mtxtextview.h"
#include "mtxviewer.h"
#include "mtxversion.h"

/* *INDENT-OFF* */
/**
usage:
*/
static void
usage ()
{
    g_print ("%s\n\n", _("Usage: mdview [OPTIONS] PATH [HOMEPAGE [TITLE]]"));
    g_autofree gchar *a = g_strdup_printf (_("\
PATH is the file or directory to open. HOMEPAGE is the default file to open\n\
if PATH is a directory. If HOMEPAGE is empty \"%1$s\" is opened.\n\
Relative paths in markdown begin at directory PATH, and if not found,\n\
at the folder of the currently viewed file.\n\
If TITLE is empty \"%2$s\" is used for the window title."),
    DEFAULT_INDEX, DEFAULT_WINDOW_TITLE);
    g_print ("%s\n", a);

#ifdef MTX_TEXT_VIEW_DEBUG
    g_print ("\n%s\n", _("INPUT TWEAKS"));
    g_print (_("\
 --markup=FILE   load pango FILE (skips markdown parser) (debug)\n"));
#endif
    g_print ("\n%s\n", _("OUTPUT FORMATS"));
    g_print ("%s\n", _("\
If an output format is not specified the file is opened in the GUI viewer.\n\
Some formats can be tweaked (see section OUTPUT FORMAT TWEAKS).\n\
\n\
 --ansi          text with ANSI escape codes\n\
 --html          HTML fragment\n\
 --text          plain text\n\
 --tty           text with vt100 codes"));
#ifdef MTX_DEBUG
    g_print (_("\
 --pango         raw pango markup (debug)\n"));
#endif
    g_print ("\n%s\n", _("OUTPUT FORMAT TWEAKS"));
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
    g_print ("%s\n", _("\
 --cm-block-end  add an empty line at code block end"));
#endif
    g_print ("%s\n", _("\
 --html5         output HTML5 rather than XHTML\n\
 --soft-break    start a new line at each soft break, a line ending inside a\n\
                 markdown paragraph, instead of joining lines with a space\n\
 --unsafe-html   include raw HTML in HTML output fragment"));

    g_print ("\n%s\n", _("EXTENSIONS"));
    g_print ("%s\n", _("\
Options that start with \"--no\" are enabled by default.\n\
\n\
 --auto-lang     prefer opening File.$LANG.ext over File.ext\n\
                 auto-language is only supported for the GUI viewer\n\
 --no-auto-code  disable rendering certain plain words as code spans\n\
 --no-extensions disable all extensions\n\
 --no-permlink   disable permissive auto-links (linkifying unmarked links)\n\
 --no-shebang    disable shebang detection\n\
 --no-smart      disable smart text replacement\n\
                 smart text works better without --soft-breaks\n\
 --no-table      disable support for markdown tables"));

    g_print ("\n%s\n", _("MISCELLANEOUS"));
    g_print ("%s\n", _("\
 --version       print version and license information and exit"));

    g_print ("\n%s\n", _("DEPRECATED"));
    g_print ("%s\n", _("\
These options could go away at the next release -- use substitutes from above\n\
\n\
 --no-ext        DEPRECATED, like --no-auto-code, --no-permlink, --no-shebang\n\
                 and --no-table all combined"));
    /*
    Note: smart text works better without soft breaks because paired quotes
    that are located in different lines can still be paired, while with soft
    breaks quotes can only be paired together if they are in the same line.
    Soft breaks are enabled by default.
    */
}
/* *INDENT-ON* */

/**
stdout_output:
Main function for text output modes.
*/
static void
stdout_output (gchar *dir,
               gchar *file,
               int output_type,
               guint extensions,
               guint tweaks)
{
    g_autofree gchar *path = NULL;
    g_autofree gchar *contents = NULL;
    g_autofree gchar *textout = NULL;
    g_autoptr (MtxCmm) markdown = mtx_cmm_new ();

    /* we do assume UTF-8 encoding */
    path = g_build_filename (dir, file, NULL);

    mtx_cmm_set_output (markdown, output_type);
    mtx_cmm_set_extensions (markdown, extensions);
    mtx_cmm_set_tweaks (markdown, tweaks);
    if (output_type == MTX_CMM_OUTPUT_PANGO)   /* --pango */
    {
        mtx_cmm_set_escape (markdown, TRUE);
    }

    contents = _get_file_contents (path, NULL, TRUE);
    if (contents != NULL)
    {
        /* UTF-8 encoding was validated */
        gsize size;

        textout = mtx_cmm_mtx (markdown, &contents, &size, TRUE);
        write (1, textout, size);
        write (1, "\n", 1);
    }
}

/**
main:
*/
int
main (int argc, char **argv)
{
    gchar *startup_file = NULL;
    gchar *home = NULL;
    gchar *title = NULL;
    g_autofree gchar *dir = NULL;
    g_autofree gchar *file = NULL;
    gboolean console_output = FALSE;
    guint extensions = 0xffff;
    extensions &= ~MTX_CMM_EXTENSION_AUTO_LANG;
    guint tweaks = 0;
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
    tweaks &= ~MTX_CMM_TWEAK_CM_BLOCK_END;
#else
    tweaks |= MTX_CMM_TWEAK_CM_BLOCK_END;
#endif
    MtxCmmOutput output_type = MTX_CMM_OUTPUT_TTY;
    gint i;
    gchar *temp;

    /* locale init */
    setlocale (LC_ALL, "");
    if ((temp = getenv ("TEXTDOMAIN")))
    {
        textdomain (temp);
    }
    if ((temp = getenv ("TEXTDOMAINDIR")))
    {
        bindtextdomain (textdomain (NULL), temp);
    }

    for (i = 1; i < argc; i++)
    {
        if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
        {
            usage ();
            exit (0);
        }
        else if (strcmp (argv[i], "-V") == 0 ||
                 strcmp (argv[i], "--version") == 0)
        {
            g_print ("%s\n", MDVIEW_VERSION_TEXT);
            exit (0);
        }
        else if (strcmp (argv[i], "--ansi") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_ANSI;
            continue;
        }
        else if (strcmp (argv[i], "--tty") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_TTY;
            continue;
        }
        else if (strcmp (argv[i], "--text") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_TEXT;
            continue;
        }
        else if (strcmp (argv[i], "--html") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_HTML;
            continue;
#ifdef MTX_DEBUG
        /*
        Use `pango-view` to validate or study pango markup, e.g.
        : markdown to markup; mdview --pango f.md > f.xml
        : validate markup   ; pango-view -q --markup f.md f.xml
        : markup to json    ; pango-view -q --serialize-to=f.json f.xml
        Note that pango markup is output before entering MtxTextView. Hence
        text isn't indented, and link and image references aren't yet resolved.
        */
        }
        else if (strcmp (argv[i], "--pango") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_PANGO;
            continue;
#endif
        }
#ifdef MTX_TEXT_VIEW_DEBUG
        else if (strncmp (argv[i], "--markup=", sizeof "--markup=" - 1) == 0)
        {
            extern gchar *gl_text_view_debug_markup_file;
            gl_text_view_debug_markup_file = argv[i] + sizeof "--markup=" - 1;
            continue;
        }
#endif
        else if (strcmp (argv[i], "--auto-lang") == 0)
        {
            extensions |= MTX_CMM_EXTENSION_AUTO_LANG;
        }
        else if (strcmp (argv[i], "--no-auto-code") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_AUTO_CODE;
            continue;
        }
        else if (strcmp (argv[i], "--no-shebang") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_SHEBANG;
            continue;
        }
        else if (strcmp (argv[i], "--no-smart") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_SMART_TEXT;
            continue;
        }
        else if (strcmp (argv[i], "--no-table") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_TABLE;
            continue;
        }
        else if (strcmp (argv[i], "--no-ext") == 0) /* DEPRECATED */
        {
            /* compatibility with legacy mdview */
            extensions &= ~(MTX_CMM_EXTENSION_SHEBANG |
                            MTX_CMM_EXTENSION_TABLE |
                            MTX_CMM_EXTENSION_AUTO_CODE |
                            MTX_CMM_EXTENSION_PERMLINK);
            continue;
        }
        else if (strcmp (argv[i], "--no-extensions") == 0)
        {
            extensions = 0;
            continue;
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
        }
        else if (strcmp (argv[i], "--cm-block-end") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_CM_BLOCK_END;
            continue;
#endif
        }
        else if (strcmp (argv[i], "--html5") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_HTML5;
            continue;
        }
        else if (strcmp (argv[i], "--unsafe-html") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_UNSAFE_HTML;
            continue;
        }
        else if (strcmp (argv[i], "--soft-break") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_SOFT_BREAK;
            continue;
        }
        else if (strcmp (argv[i], "--soft-breaks") == 0) /* DEPRECATED */
        {
            tweaks |= MTX_CMM_TWEAK_SOFT_BREAK;
            continue;
        }
        else if (strcmp (argv[i], "--no-permlink") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_PERMLINK;
            continue;
        }
        else if (argv[i][0] == '-')
        {
            usage ();
            fprintf (stderr, "%s: %s %s\n", PROGNAME, _("invalid option:"),
                     argv[i]);
            exit (1);
        }
        else if (startup_file == NULL)
        {
            startup_file = argv[i];
        }
        else if (home == NULL)
        {
            home = argv[i];
        }
        else if (title == NULL)
        {
            title = argv[i];
        }
    }

    /* defaults */
    if (home != NULL && home[0] == '\0')
    {
        home = NULL;
    }
    if (title != NULL && title[0] == '\0')
    {
        title = NULL;
    }
    if (startup_file != NULL)
    {
        if (g_file_test (startup_file, G_FILE_TEST_IS_DIR))
        {
            dir = g_strdup (startup_file);
            file = g_path_get_basename (home != NULL ? home : DEFAULT_INDEX);
        }
        else
        {
            dir = g_path_get_dirname (startup_file);
            file = g_path_get_basename (startup_file);
        }
    }
    else
    {
        usage ();
        exit (1);
    }

    /* we do assume UTF-8 encoding */
    startup_file = g_build_filename (dir, file, NULL);
    if (!g_file_test (startup_file, G_FILE_TEST_EXISTS))
    {
        g_printerr ("%s: '%s': %s\n", PROGNAME, startup_file,
                    strerror (ENOENT));
        g_free (startup_file);
        exit (1);
    }
    g_free (startup_file);

    if (console_output || (temp = getenv ("DISPLAY")) == NULL
        || temp[0] == '\0')
    {
        /* output to stdout */
        stdout_output (dir, file, output_type, extensions, tweaks);
    }
    else
    {
        MtxViewer *mvr;

        gtk_init (&argc, &argv);
        mvr = mtx_viewer_new (dir, file, title, NULL, extensions, tweaks);
        if (mvr == NULL)
        {
            exit (1);
        }
        mvr->homepage = file;
        gtk_main ();
    }
}

