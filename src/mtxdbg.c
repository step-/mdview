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

#ifdef MTX_DEBUG

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

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

int mtx_dbg_dprint (int, int, const char *, ...)
__attribute__((format(printf, 3, 4))); /* 3=format 4=params */

int
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

/**
mtx_dbg_etime:
Return time elapsed since the last call.

@elapse: 0 resets the base timer to "now"; CAVEAT: there's only
one base timer. 1 calculates the elapsed time since the last call.

Returns: 0 if @elapse == 0; otherwise it returns a double
representing seconds and fractional seconds since the last call.
*/
double
mtx_dbg_etime (unsigned elapse)
{
    static struct timespec prev = {0};
    struct timespec now;
    double delta;

    if (elapse)
    {
        clock_gettime (CLOCK_MONOTONIC, &now);
        delta = now.tv_sec - prev.tv_sec + (now.tv_nsec - prev.tv_nsec) / 1e9;
        prev = now;
    }
    else
    {
        clock_gettime (CLOCK_MONOTONIC, &prev);
        delta = 0;
    }
    return delta;
}

/**
mtx_dbg_fmt_etime:

@s: elapsed time (seconds) to format. If @s < 0 call
`mtx_dbg_etime` to get the current elapsed time then format it.

Return: pointer to a statically-allocated string holding @s as "mm:ss.fff".
*/
const char *
mtx_dbg_fmt_etime (double s)
{
    static char f[64] = "";
    if (s < 0)
    {
        s = mtx_dbg_etime (1);
    }
    int m = (int) (s / 60);
    double r = s - (m * 60);
    snprintf (f, 64, "%02d:%02d.%03d", m, (int) r, (int) ((r - (int) r) *1000));
    return f;
}

/**
mtx_dbg_tally_ary:
32 integer counters
*/
unsigned int mtx_dbg_tally_ary[32] = {0};

#endif /* MTX_DEBUG */
