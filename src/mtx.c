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

#include <ctype.h>
#include "mtx.h"
#include "mtxcmmprivate.h"
#include "mtx.decl.h"

/**
mtx_word_is_ident:
@up: option uppercase only
Return: length of matched indentifier [[:alnum:]_]+
- first character not digit
- underscore can be escaped with backslash
*/
static inline gint
mtx_word_is_ident (const gchar *s,
                   gsize len,
                   gboolean up)
{
    gsize i;

    if (isdigit (*s))
    {
        return 0;
    }
    for (i = 0; i < len; i++)
    {
        switch (s[i])
        {
        case '\\':
        case '_':
            continue;
        }
        if (up ? isdigit (s[i]) || isupper (s[i]) : isalnum (s[i]))
        {
            continue;
        }
        break;
    }
    return i;
}

/**
mtx_word_type:
@text: markdown text tighly wrapping a word.
@start: pointer to starting position in @text; also a return value.
@length: pointer to length of segment in bytes; also a return value.
Input: If *@start is >= 0 it is the starting position of the segment to be assessed.
Input: If *@length is >= 0 it is the length of the segment to be assessed.
Return value: `MtxCmmWordType` type of the matched word.
Output: If @start is not NULL return the starting position of the matched word.
Output: If @length is not NULL return the length of the match.
*/
MtxCmmWordType
mtx_word_type (const gchar *text,
               gint *start,
               gint *length)
{
    g_return_val_if_fail (text && start && length
                          && *length > 3, MTX_CMM_WORD_UNKNOWN);

    MtxCmmWordType ret = MTX_CMM_WORD_UNKNOWN;
    gchar *s, *e; /* return-word *s(tart) and *e(nd) */
    gint i, cwl;  /* return-word length */

    const gchar *s0 = *start >= 0 ?  text + *start : text;
    const gchar *e0 = *length >= 0 ? text + *start + *length : text + *length;

    s   = (gchar *)s0;
    e   = (gchar *)e0;
    cwl = (ptrdiff_t) (e - s) + 1;

/*
Invariant: the return-word candidate <s>...<e> will be cwl chars long,
for s in range [s0..e], e in range [s..e0].
Test Nth character before / after current return-word start / end.
*/
#define LFLANK(N, C) (((ptrdiff_t)(s - s0) > 0) && (*(s - N) == (C)))
#define RFLANK(N, C) (((ptrdiff_t)(e0 - e) > 0) && (*(e + N) == (C)))

    /* Dequote. */
    /* '"' can't happen because it's word-separator rUNIPUA_QUOT */
    if (*s == '\'' && *e == '\'' /* || *s == '"' && *e == '"' */)
    {
      s++, e--;
      cwl -= 2;
      if (cwl < 4)
      {
        goto out;
      }
    }

    /* URI can be non-ASCII */
    /* https://en.wikipedia.org/wiki/Internationalized_domain_name */
    {
        gchar **x, *ext[] = { "https://", "http://", "ftp://", NULL };
        gint len;
        for (x = ext; *x; x++)
        {
            len = strlen (*x);
            if (!strncmp (s, *x, len))
            {
                ret = MTX_CMM_WORD_URI;
                goto out;
            }
            /* [(]http://...[)] */
            if (cwl >= len + 2 && !strncmp (s + 1, *x, len - 1) && *s == '('
                && *e == ')')
            {
                s++, e--;
                cwl -= 2;
                ret = MTX_CMM_WORD_URI_FLANKED;
                goto out;
            }
        }
    }

    /******************************************/
    /* ignore trailing punctuation except '_' */
    /******************************************/
    if (e[-1] != '\\')
    {
        for (i = cwl; i > 0 && *e != '_' && ispunct (*e);)
        {
            e--, i--;
        }
        cwl = i;
    }

/* *INDENT-OFF*/
    /* pathname length >= 4 bytes */
    /* (absolute path or path starting with "./" or "../") */
    if (cwl >= 4 && (*s == '/' || (s[0] == '.' && (s[1] == '/' ||
        (s[1] == '.' && (s[2] == '/' || (s[2] == '.' && s[3] == '/')))))))
    {
        if (RFLANK(1, '/'))
        {
          e++, cwl++;
        }
        ret = MTX_CMM_WORD_ABS_PATH;
        goto out;
    }
/* *INDENT-ON* */

    /*****************************************/
    /* ignore leading punctuation except '_' */
    /*****************************************/
    if (s == s0 || s[-1] != '\\')
    {
        for (i = cwl; i > 0 && *s != '_' && ispunct (*s); )
        {
          s++, i--;
        }
        cwl = i;
    }
    if (cwl <= 0) /* nothing to do */
    {
        goto out;
    }

    /* match file name != "" + extension */
    {
        gchar **x, *ext[] = { ".patch", ".diff", NULL };
        gint len;
        for (x = ext; *x; x++)
        {
            len = strlen (*x);
            if (cwl > len && !strncmp (e - len + 1, *x, len))
            {
                ret = MTX_CMM_WORD_FILE_DIFF;
                goto out;
            }
        }
    }

    /*************************************************************
    *                  ASCII only from here on                   *
    *************************************************************/
    for (i = 0; i < cwl; i++)
    {
        if (!isascii (s[i]))
        {
            goto out;
        }
    }

    /* bugzillas */
    if (LFLANK (1, '#')
        && !(LFLANK (2, '[') || LFLANK (2, '<') || LFLANK (2, '(')))
    {
        s--, cwl++;
        ret = MTX_CMM_WORD_BUGZILLA;
        goto out;
    }

    /* function name followed by "()" no spaces */
    if (RFLANK (1, '(') && RFLANK (2, ')') &&
        mtx_word_is_ident (s, cwl, FALSE) == cwl)
    {
        e = e + 2;
        cwl += 2;
        ret = MTX_CMM_WORD_FUNCNAME;
        goto out;
    }

    /* email addresses */
    if (*s != '@' && *s != '.' && *e != '@')
    {
        guint dot = 0, at = 0;
        for (i = 0; i < cwl; i++)
        {
            switch (s[i])
            {
            case '.': ++dot; break;
            case '@': ++at;  break;
            }
        }
        if (at == 1 && dot > 0)
        {
            /* kludge to prevent entangling an autolink */
            if (LFLANK (1, '[') && RFLANK (1, ']'))
            {
                goto out;
            }
            ret = MTX_CMM_WORD_EMAIL;
            goto out;
        }
    }

    /* uppercase identifier that includes at least one '_' */
    if (mtx_word_is_ident (s, cwl, TRUE) == cwl && memchr (s, '_', cwl))
    {
        if (LFLANK (1, '$'))
        {
            s--, cwl++;
        }
        ret = MTX_CMM_WORD_UIDENT;
        goto out;
    }

out:
    *start = (ptrdiff_t) (s - text);
    *length = cwl;
    return ret;
}

/**
mtx_cmm_str_slugify:

@str: pure text string (markdown removed)
@sep: separator character that will replace spaces
*/
/*
Our slugify is equivalent to pandoc's auto_identifiers+ascii_identifiers.
The spec in `pandoc --help` version 3.3 says (my notes in ALL CAPS):
Extension: auto_identifiers
   A heading without an explicitly specified identifier will be
   automatically assigned a unique identifier based on the heading text.
   The default algorithm used to derive the identifier from the heading text is:

   * Remove all formatting, links, etc.    @str PRECONDITION
   * Remove all footnotes.                 NOT AVAILABLE
   * Remove all non-alphanumeric characters, except underscores, hyphens, and
     periods.
   * Replace all spaces and newlines with hyphens.
   * Convert all alphabetic characters to lowercase.
   * Remove everything up to the first letter (identifiers may not begin with a
     number or punctuation mark).
   * If nothing is left after this, use the identifier "section".

Thus, for example,

Heading                       Identifier
----------------------------- -----------------------------
Heading identifiers in HTML   heading-identifiers-in-html
Maître d'hôtel                maître-dhôtel
*Dogs*?--in *my* house?       dogs--in-my-house
[HTML], [S5], or [RTF]?       html-s5-or-rtf
3. Applications               applications
33                            section

NOT IMPLEMENTED
These rules should, in most cases, allow one to determine the identifier from
the heading text. The exception is when several headings have the same text;
in this case, the first will get an identifier as described above; the second
will get the same identifier with -1 appended; the third with -2; and so on.

Extension: ascii_identifiers
   Causes the identifiers produced by auto_identifiers to
   be pure ASCII. Accents are stripped off of accented
   Latin letters, and non-Latin letters are omitted.

IN ACTUAL RUNS (test/pandoc_auto_identifiers.sh) PANDOC 3.3 ALSO DOES:
  * SQUEEZE INTERIOR SPACES.
  * TRIM TRAILING SPACES.
*/
gchar *
mtx_str_slugify (const gchar *str,
                 const gchar sep)
{
    gboolean squeezing;
    gchar *p, *q;
    gchar *a = g_str_to_ascii (str, "C");
    for (p = q = a; *p; p++)
    {
        if (g_ascii_isalnum (*p))
        {
            *q++ = g_ascii_tolower (*p);
        }
        else if G_UNLIKELY
            (*p == '_' || *p == '-' || *p == '.' ||
             *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        {
            *q++ = *p;
        }
    }
    *q = '\0';
    while (--q >= a && *q == ' ')
        *q = '\0';
    for (p = a; *p && !g_ascii_isalpha (*p); p++)
        ;
    for (q = a, squeezing = FALSE; *p; p++)
    {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        {
            if (!squeezing)
            {
                *q++ = sep;
                squeezing = TRUE;
            }
        }
        else
        {
            *q++ = *p;
            squeezing = FALSE;
        }
    }
    *q = '\0';
    return a;
}

/**
mtx_str_delete_unipua_em_strong:
Remove UNIPUA code points for markdown * and **.

@str: C string to be modified in place.
*/
void
mtx_str_delete_unipua_em_strong (gchar *str)
{
    gchar *p = str;
    gchar *z = strchr (str, '\0');
    do
    {
        if (*p++ == cUNIPUA0 && *p++ == cUNIPUA1)
        {
            switch (*p++)
            {
                /* unlikely */
                case cUNIPUA_E1:
                case cUNIPUA_E0:
                case cUNIPUA_B1:
                case cUNIPUA_B0:
                    memmove (p - 3, p, z - p + 1);
                    p -= 3, z -= 3;
                    break;
            }
        }
    }
    while (*p);
}

