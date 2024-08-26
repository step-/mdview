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

#ifndef MTX_DBG_H
#define MTX_DBG_H

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef MTX_DEBUG

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

/**
mtx_dbg_dprint:
Conditional dprintf(...) function.

@level: integer, only print if `#MTX_DEBUG` is at least @level. Pass -1 to
disable printing altogether.
@fd: file descriptor, pass -1 for stderr.
@fmt: see printf(1).
...: print arguments.

This function formats and prints its print arguments to file descriptor @fd.

Returns: the number of bytes printed.
*/
/**
mtx_dbg_errseq:
Conditional dprintf(2, ...) macro.
@level: see #mtx_dbg_dprint.
@fmt: see #mtx_dbg_dprint.
...: print arguments.

This macro prints its print arguments to stderr using #mtx_dbg_dprint.
See also #mtx_dbg_errout.
*/
/**
mtx_dbg_errout:
Conditional dprintf(2, ...) macro.

Like #mtx_dbg_errseq. The print out is preceded by the concatenation of @level,
the name of the calling function, and ": ". You would most often use this
macro alone or before a series of #mtx_dbg_errseq macros to output a line
incrementally.
*/


static int mtx_dbg_dprint (int, int, const char *, ...)
__attribute__((unused, format(printf, 3, 4))); /* 3=format 4=params */

static int
mtx_dbg_dprint (int level, int fd, const char *fmt, ...)
{
    int ret = 0;
    if (level <= MTX_DEBUG)
    {
        va_list args;
        va_start (args, fmt);
        ret = vdprintf (fd < 0 ? STDERR_FILENO : fd, fmt, args);
        va_end (args);
    }
    return ret;
}

#define mtx_dbg_dprint(level, ...)                                     \
    ((level) < 0 ? (void)0 : mtx_dbg_dprint (level, __VA_ARGS__))

#define mtx_dbg_errout(level, fmt, ...)                                \
    mtx_dbg_dprint(level, STDERR_FILENO, "|>%d:%s: " fmt,              \
                   level, __FUNCTION__, __VA_ARGS__)

#define mtx_dbg_errseq(level, fmt, ...)                                \
    mtx_dbg_dprint(level, STDERR_FILENO, fmt, __VA_ARGS__)

#else  /* MTX_DEBUG */

#define mtx_dbg_dprint(...)
#define mtx_dbg_errout(...)
#define mtx_dbg_errseq(...)

#endif /* MTX_DEBUG */

#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif /* MTX_DBG_H */
