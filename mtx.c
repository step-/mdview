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

    /* absolute path length >= 4 bytes */
    if (cwl >= 4 && *s == '/')
    {
        if (RFLANK(1, '/'))
        {
          e++, cwl++;
        }
        ret = MTX_CMM_WORD_ABS_PATH;
        goto out;
    }

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
    if (LFLANK(1, '#'))
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

