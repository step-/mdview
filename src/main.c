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

#include <glib/gi18n.h>        /* xgettext --keyword=_ --keyword=Q_:1g */
#include <locale.h>
#include <fcntl.h>

#include "mtxresources.h"
#include "mtxtextview.h"
#include "mtxviewer.h"
#include "mtxversion.h"
#include "main.decl.h"

/**
fd_output:
Main function for text output modes.

@fd: output file descriptor.
@dir: @file's.
@file: file name to open.
@output_type: #MtxCmmOutput.
@extensions: #MtxCmmExtensions flags.
@html_base: URI for `<base>` HTML tag, NULLABLE.
@html_css: CSS style sheet id for HTML output.
@toc_level: table of contents maximum level.
@tweaks: #MtxCmmTweaks flags.

Returns: -1 on error.
*/
static int
fd_output (const gint out_fd,
           gchar *dir,
           gchar *file,
           MtxCmmOutput output_type,
           MtxCmmExtensions extensions,
           const gchar *html_base,
           gint html_css,
           guint toc_level,
           MtxCmmTweaks tweaks)
{
    g_autofree gchar *path = NULL;
    g_autofree gchar *contents = NULL;
    g_autoptr (GBytes) bytes = NULL;
    g_autofree gchar *textout = NULL;
    g_autoptr (MtxCmm) markdown = mtx_cmm_new (MTX_CMM_OUTPUT_UNKNOWN);
    const gchar *scheme = g_uri_peek_scheme (file);

    if (scheme == NULL)                /* disk file */
    {
        path = g_build_filename (dir, file, NULL);
        if (!g_file_test (path, G_FILE_TEST_EXISTS))
        {
            g_printerr ("%s: '%s': %s\n", PROGNAME, path, strerror (ENOENT));
            return -1;
        }
        contents = mtx_text_view_mmap_read_file (path, NULL, TRUE);
    }
    else if (strcmp (scheme, "resource") == 0)     /* embedded resource */
    {
        bytes =
        g_resources_lookup_data (file + sizeof "resource://" -1, 0, NULL);
        if (bytes != NULL)
        {
            contents = g_strdup (g_bytes_get_data (bytes, NULL));
        }
    }
    /* Dev: a new scheme added here must be added to is_valid_scheme too. */

    if (contents != NULL)
    {
        /* UTF-8 encoding was validated */
        gsize size;

        if (output_type == MTX_CMM_OUTPUT_UNKNOWN) /* set by --dump-styles */
        {
            write (out_fd, contents, strlen (contents));
            write (out_fd, "\n", 1);
            textout = contents;
            contents = NULL;
        }
        else
        {
            if (output_type == MTX_CMM_OUTPUT_HTML
                && (tweaks & MTX_CMM_TWEAK_FULL_HTML))
            {
                html_doc_start (out_fd, tweaks, html_css, html_base ?
                                (*html_base ? html_base : NULL) : dir);
                /* recursive */
            }

            mtx_cmm_set_extensions (markdown, extensions);
            mtx_cmm_set_tweaks (markdown, tweaks);
            mtx_cmm_set_output (markdown, output_type);
            if (output_type == MTX_CMM_OUTPUT_PANGO)   /* --pango */
            {
                mtx_cmm_set_escape (markdown, TRUE);
            }
            mtx_cmm_set_toc_level (markdown, toc_level);

            /* TODO allow cancelling text mode output. */
            textout =
            mtx_cmm_mtx (markdown, &contents, &size, NULL, NULL, TRUE, NULL);
            write (out_fd, textout, size);
            write (out_fd, "\n", 1);

            if (output_type == MTX_CMM_OUTPUT_HTML
                && (tweaks & MTX_CMM_TWEAK_FULL_HTML))
            {
                html_doc_end (out_fd, tweaks, html_css, html_base == NULL ||
                              html_base[0]);
            }
        }
    }
    return textout != NULL ? 0 : -1;
}

/**
html_doc_start:

@fd: output file descriptor.
@tweaks: pointer to #MtxCmmTweaks.
@html_css: CSS stylesheet id.
@html_base: the base URI for the HTML <base> tag to be inserted in
the <head> tag, NULLABLE. If NULL, the <base> tag won't be inserted.
*/
static void
html_doc_start (const gint fd,
                MtxCmmTweaks tweaks,
                const gint html_css,
                const gchar *html_base)
{
/* https://www.w3.org/International/questions/qa-html-encoding-declarations */
/* *INDENT-OFF* */
    gboolean html5 = tweaks & MTX_CMM_TWEAK_HTML5;
    dprintf (fd, html5 ?
    "<!DOCTYPE html>\n<html lang=\"en\">"
    "\n<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    :
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
    "\n<head>\n"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"
    );
    dprintf (fd, "\n<title>%s</title>\n", "TITLE");
    if (html_base != NULL)
    {
        dprintf (fd, "<base href=\"%s/\"%s\n", html_base, html5 ? ">" : " />");
    }
    if (html_css > 0)
    {
        gchar sheet[sizeof STYLE_SHEET_PAGE_FMT + 8];

        dprintf (fd, html5 ? "<style>\n" : "<style type=\"text/css\">\n");
        snprintf (sheet, sizeof sheet, STYLE_SHEET_PAGE_FMT, html_css);
        fd_output (fd, "", sheet, MTX_CMM_OUTPUT_UNKNOWN, 0, NULL, 0, 0, 0);
        dprintf (fd, "</style>\n");
    }
    dprintf (fd, "</head>\n<body>\n<div class=\"mtx-body\">\n");
/* *INDENT-ON* */
}

/**
html_doc_end:

@fd: output file descriptor.
@tweaks: pointer to #MtxCmmTweaks.
@html_css: CSS stylesheet id.
@html_base: the base URI for the HTML <base> tag to be inserted in
the <head> tag, NULLABLE. If NULL, the <base> tag won't be inserted.
*/
static void
html_doc_end (const gint fd,
              MtxCmmTweaks tweaks,
              const gint html_css __attribute__((unused)),
              const gboolean add_js)
{
    if (!add_js)
    {
        dprintf (fd, "</div>\n</body>\n</html>\n");
    }
    else
    {
        gboolean html5 = tweaks & MTX_CMM_TWEAK_HTML5;
        /* *INDENT-OFF* */
        dprintf (fd, "</div>\n<script id=\"mtx_js_1\"%s>\n"
        /* For internal links to work together with the <base>
        tag in this document we need to prepend the document path
        to the internal links with this javascript script. */
 "// adjust internal link fragments\n"
"(function() {\n"
"var stem=document.location.origin+document.location.pathname;\n"
"var base=document.getElementsByTagName('base');\n"
"base=base[0].href;\n"
"var links=document.getElementsByTagName('a');\n"
"for(let i = 0; i < links.length; i++) {\n"
"    if(links[i].href.indexOf(base+'#')==0) {\n"
"        links[i].href=stem+links[i].href.slice(links[i].href.indexOf('#'));\n"
"    }\n"
"}\n"
"})();\n"
                 "</script>\n</body>\n</html>\n", html5 ? "" :
                 " type=\"text/javascript\"");
        /* *INDENT-ON* */
    }
}

/**
usage:
*/
static void
usage (MtxCmmOutput output)
{
    if (output != MTX_CMM_OUTPUT_BARE)
    {
        output = MTX_CMM_OUTPUT_ANSI;
    }
    (void) fd_output (STDOUT_FILENO, "", USAGE_PAGE, output, 0, NULL, 0, 0, 0);
}

/**
is_valid_scheme:

@scheme: string, URI scheme
@console_output: whether output is going to the console.

Return TRUE if @scheme is supported for console or viewer output.
*/
static gboolean
is_valid_scheme (const char *scheme,
                 const gboolean console_output)
{
    return scheme != NULL && (strcmp (scheme, "resource") == 0 ||
                              (!console_output
                               && strcmp (scheme, "search") == 0));
}

/**
main:
*/
int
main (int argc, char **argv)
{
    const gchar *homepath = NULL;
    const gchar *homepage = NULL;
    const gchar *title = NULL;
    g_autofree gchar *dir = NULL;
    g_autofree gchar *file = NULL;
    gboolean console_output = FALSE;
    guint extensions = MTX_CMM_EXTENSION_DEFAULT;
    guint toc_level = 0;
    const gchar *html_base = NULL;
    gint html_css = -1;
    g_autofree gchar *dump_css = NULL;
    guint tweaks = MTX_CMM_TWEAK_NONE;
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
    tweaks &= ~MTX_CMM_TWEAK_CM_BLOCK_END;
#else
    tweaks |= MTX_CMM_TWEAK_CM_BLOCK_END;
#endif
    MtxCmmOutput output_type = MTX_CMM_OUTPUT_TTY;
    guint fd_out = STDOUT_FILENO;
    gint i;

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /*
    Parse options, prepending $MDVIEW_OPTION to the command line.
    */
    g_autofree GStrv args = NULL;
    g_autoptr (GPtrArray) argp = NULL;
    gint narg = 0;

    if (g_getenv ("MDVIEW_OPTIONS") != NULL)
    {
        g_autoptr (GError) err = NULL;
        if (!g_shell_parse_argv (g_getenv ("MDVIEW_OPTIONS"), &narg, &args,
                                 &err))
        {
            g_printerr (_("MDVIEW_OPTIONS is invalid: %s\n"), err->message);
            exit (1);
        }
    }
    argp = g_ptr_array_new_full (narg + argc - 1, NULL);
    for (i = 0; i < narg; i++)
    {
        const gchar **aptr = (const gchar **) &g_ptr_array_index (argp, i);
        *aptr = args[i];
    }
    for (i = 1; i < argc; i++)
    {
        const gchar **aptr =
        (const gchar **) &g_ptr_array_index (argp, narg - 1 + i);
        *aptr = argv[i];
    }
    for (i = 0; i < narg + argc - 1; i++)
    {
        const gchar *arg = g_ptr_array_index (argp, i);

        if (strcmp (arg, "-h") == 0 || strcmp (arg, "--help") == 0)
        {
            usage (output_type);
            exit (0);
        }
        else if (strcmp (arg, "-V") == 0 ||
                 strcmp (arg, "--version") == 0)
        {
            gchar *_ __attribute__((unused)) = _("Version:");
            g_print ("%s\n", MDVIEW_VERSION);
            exit (0);
        }
        else if (strcmp (arg, "--license") == 0)
        {
            g_print (MDVIEW_LICENSE "\n", _("License:"));
            exit (0);
        }
        else if (strncmp (arg, "--output=", sizeof "--output=" - 1) == 0)
        {
            const gchar *outf = arg + sizeof "--output=" - 1;
            gint fd = open (outf, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR |
                            S_IRUSR | S_IRGRP | S_IROTH);
            if (fd < 0)
            {
                gint n = errno;
                usage (output_type);
                g_printerr ("%s: %s: %s\n", PROGNAME, outf, g_strerror (n));
                exit (1);
            }
            fd_out = fd;
            continue;
        }
        else if (strcmp (arg, "--ansi") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_ANSI;
            continue;
        }
        else if (strcmp (arg, "--tty") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_TTY;
            continue;
        }
        else if (strcmp (arg, "--bare") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_BARE;
            continue;
        }
        else if (strcmp (arg, "--text") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_TEXT;
            continue;
        }
        else if (strcmp (arg, "--html") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_HTML;
            continue;
#ifdef OPT_PANGO
        /*
        Use `pango-view` to validate or study pango markup, e.g.
        : markdown to markup; mdview --pango f.md > f.xml
        : validate markup   ; pango-view -q --markup f.md f.xml
        : markup to json    ; pango-view -q --serialize-to=f.json f.xml
        Note that pango markup is output before entering MtxTextView. Hence
        text isn't indented, and link and image references aren't yet resolved.
        */
        }
        else if (strcmp (arg, "--pango") == 0)
        {
            console_output = TRUE;
            output_type = MTX_CMM_OUTPUT_PANGO;
            tweaks |= MTX_CMM_TWEAK_RESERVED1;
            continue;
#endif
        }
#ifdef OPT_MARKUP
        else if (strncmp (arg, "--markup=", sizeof "--markup=" - 1) == 0)
        {
            extern const gchar *gl_text_view_debug_markup_file;
            gl_text_view_debug_markup_file = arg + sizeof "--markup=" - 1;
            continue;
        }
#endif
        else if (strcmp (arg, "--auto-lang") == 0)
        {
            extensions |= MTX_CMM_EXTENSION_AUTO_LANG;
        }
        else if (strcmp (arg, "--no-auto-code") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_AUTO_CODE;
            continue;
        }
        else if (strcmp (arg, "--no-heading-link") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_HEADING_LINK;
            continue;
        }
        else if (strcmp (arg, "--no-shebang") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_SHEBANG;
            continue;
        }
        else if (strcmp (arg, "--no-smart") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_SMART_TEXT;
            continue;
        }
        else if (strcmp (arg, "--no-strikethrough") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_STRIKETHROUGH;
            continue;
        }
        else if (strcmp (arg, "--no-table") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_TABLE;
            continue;
        }
        else if (strcmp (arg, "--no-ext") == 0) /* DEPRECATED */
        {
            /* compatibility with legacy mdview */
            extensions &= ~(MTX_CMM_EXTENSION_SHEBANG |
                            MTX_CMM_EXTENSION_TABLE |
                            MTX_CMM_EXTENSION_AUTO_CODE |
                            MTX_CMM_EXTENSION_PERMLINK);
            continue;
        }
        else if (strcmp (arg, "--no-extensions") == 0)
        {
            extensions = 0;
            continue;
        }
        else if (strncmp (arg, "--toc-level=", sizeof "--toc-level=" - 1)
                 == 0)
        {
            toc_level =
            MAX (MIN (atoi (arg + sizeof "--toc-level=" - 1) , 6), 0);
            continue;
#ifndef COMMONMARK_FENCED_CODEBLOCK_LINE_ENDING
        }
        else if (strcmp (arg, "--cm-block-end") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_CM_BLOCK_END;
            continue;
#endif
        }
        else if (strcmp (arg, "--html5") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_HTML5;
            continue;
        }
        else if (strcmp (arg, "--unsafe-html") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_UNSAFE_HTML;
            continue;
        }
        else if (strncmp (arg, "--html-base=", sizeof "--html-base=" - 1) == 0)
        {
            html_base = arg + sizeof "--html-base=" - 1;
            continue;
        }
        else if (strncmp (arg, "--html-css=", sizeof "--html-css=" - 1)
                 == 0)
        {
            html_css = atoi (arg + sizeof "--html-css=" - 1);
            tweaks |= MTX_CMM_TWEAK_FULL_HTML;
            continue;
        }
        else if (strcmp (arg, "--html-full") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_FULL_HTML;
            continue;
        }
        else if (strcmp (arg, "--soft-break") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_SOFT_BREAK;
            continue;
        }
        else if (strcmp (arg, "--soft-break-br") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_SOFT_BREAK | MTX_CMM_TWEAK_SOFT_BREAK_BR;
            continue;
        }
        else if (strcmp (arg, "--soft-breaks") == 0) /* UNDOCUMENTED */
        {
            /* compatibility with legacy mdview */
            tweaks |= MTX_CMM_TWEAK_SOFT_BREAK;
            continue;
        }
        else if (strcmp (arg, "--no-permlink") == 0)
        {
            extensions &= ~MTX_CMM_EXTENSION_PERMLINK;
            continue;
        }
        else if (strncmp (arg, "--dump-css=", sizeof "--dump-css=" - 1) == 0)
        {
            guint n = atoi (arg + sizeof "--dump-css=" - 1);
            dump_css = g_strdup_printf (STYLE_SHEET_PAGE_FMT, n);
            homepath = dump_css;
            output_type = MTX_CMM_OUTPUT_UNKNOWN;
            break; /* ignore further options */
        }
        else if (strncmp (arg, "--emask=", sizeof "--emask=" - 1) == 0)
        {
            /* UNDOCUMENTED */
            /* override extension options parsed so far */
            extensions = atoi (arg + sizeof "--emask=" - 1);
            continue;
        }
        else if (strncmp (arg, "--tmask=", sizeof "--tmask=" - 1) == 0)
        {
            /* UNDOCUMENTED */
            /* override tweak options parsed so far */
            tweaks = atoi (arg + sizeof "--tmask=" - 1);
            continue;
        }
#ifdef OPT_EXIT_TEST
        else if (strcmp (arg, "--exit-test") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_RESERVED2;
            continue;
        }
#endif
        else if (strcmp (arg, "--lint") == 0)
        {
            tweaks |= MTX_CMM_TWEAK_RESERVED4;
            continue;
        }
        else if (arg[0] == '-')
        {
            usage (output_type);
            fprintf (stderr, "%s: %s %s\n", PROGNAME, _("invalid option:"),
                     arg);
            exit (1);
        }
        else if (homepath == NULL)
        {
            homepath = arg;
        }
        else if (homepage == NULL)
        {
            homepage = arg;
        }
        else if (title == NULL)
        {
            title = arg;
        }
    }

    if (!console_output)
    {
        const gchar *display;
        if (output_type == MTX_CMM_OUTPUT_UNKNOWN ||
            (display = getenv ("DISPLAY")) == NULL || !*display)
        {
            console_output = TRUE;
        }
    }

    /******************************************
    *  Normalize HOMEPATH [HOMEPAGE [TITLE]]  *
    *******************************************/

    if (homepage != NULL && homepage[0] == '\0')
    {
        homepage = NULL;
    }
    if (title != NULL && title[0] == '\0')
    {
        title = NULL;
    }
    if (homepath != NULL && homepath[0] != '\0')
    {
        const gchar *scheme;

        if (g_file_test (homepath, G_FILE_TEST_IS_DIR))
        {
            dir = g_strdup (homepath);
        }
        else if ((scheme = g_uri_peek_scheme (homepath)))
        {
            if (is_valid_scheme (scheme, console_output))
            {
                dir = strdup ("");
                file = strdup (homepath);
            }
            else
            {
                usage (output_type);
                fprintf (stderr, "%s: '%s': unsupported scheme\n", PROGNAME,
                         scheme);
                exit (1);
            }
        }
        if (dir == NULL)
        {
            dir = g_path_get_dirname (homepath);
            file = g_path_get_basename (homepath);
        }
        if (file == NULL)
        {
            if (homepage && (scheme = g_uri_peek_scheme (homepage)))
            {
                if (is_valid_scheme (scheme, console_output))
                {
                    file = strdup (homepage);
                }
                else
                {
                    usage (output_type);
                    fprintf (stderr, "%s: '%s': unsupported scheme\n", PROGNAME,
                             scheme);
                    exit (1);
                }
            }
            else
            {
                file =
                g_path_get_basename (homepage ? homepage : DEFAULT_INDEX);
            }
        }
    }
    else
    {
        if (console_output)
        {
            usage (output_type);
            exit (1);
        }
    }

    if (console_output)
    {
        if (!(output_type & (MTX_CMM_OUTPUT_PANGO | MTX_CMM_OUTPUT_HTML)))
        {
            extensions &= ~MTX_CMM_EXTENSION_HEADING_LINK;
        }
        gint ret = fd_output (fd_out, dir, file, output_type, extensions,
                              html_base, html_css, toc_level, tweaks);
        if (close (fd_out) < 0)
        {
            perror (PROGNAME);
            ret = -1;
        }
        exit (ret < 0 ? 1 : 0);
    }
    else
    {
        MtxViewer *mvr;
        MtxViewerOptions options =
        { extensions, html_base, html_css, toc_level, tweaks };

        gtk_init (&argc, &argv);
        mvr =
        mtx_viewer_new (dir, file, title, NULL, &options);
        if (mvr == NULL)
        {
            exit (1);
        }
        gtk_main ();
    }
    exit (0);
}

