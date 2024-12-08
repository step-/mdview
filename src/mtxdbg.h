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

/* standout, standout end */
#define _SO    "\033[7m"
#define _SE    "\033[0m"
#define _SObla "\033[7;30m"
#define _SOred "\033[7;31m"
#define _SOgre "\033[7;32m"
#define _SOyel "\033[7;33;46m"
#define _SOblu "\033[7;34m"
#define _SOmag "\033[7;35m"
#define _SOcya "\033[7;36m"
#define _SOwhi "\033[7;37m"

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef MTX_DEBUG

#include <strings.h>
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


int mtx_dbg_dprint (int, int, const char *, ...)
__attribute__((format(printf, 3, 4))); /* 3=format 4=params */

#define mtx_dbg_dprint(level, ...)                                     \
    ((level) < 0 ? (void)0 : mtx_dbg_dprint (level, __VA_ARGS__))

#define mtx_dbg_errout(level, fmt, ...)                                \
    mtx_dbg_dprint(level, STDERR_FILENO, "|>%d:%s: " fmt,              \
                   level, __FUNCTION__, __VA_ARGS__)

#define mtx_dbg_errseq(level, fmt, ...)                                \
    mtx_dbg_dprint(level, STDERR_FILENO, fmt, __VA_ARGS__)

/**
mtx_dbg_tally:
This macro counts power-of-two integers.
*/
#define mtx_dbg_tally(n)           mtx_dbg_tally_ary[ffs (n)]++
extern int mtx_dbg_tally_ary[32];

/**
mtx_dbg_tally_reset:
This macro resets the counters.
*/
#define mtx_dbg_tally_reset()                                             \
do {                                                                      \
    for (unsigned int u_ = 0; u_ < 32; u_++) {                            \
        mtx_dbg_tally_ary[u_] = 0;                                        \
    }                                                                     \
} while (0)

/**
mtx_dbg_print_tally:
This macro prints mtx_dbg_tally_ary counters.

@level: see mtx_dbg_dprint.
@label: const char ** label array.
*/
#define mtx_dbg_print_tally(level, label)                                 \
do {                                                                      \
    unsigned long ctr_ = 0;                                               \
    mtx_dbg_errout (level, "%s", "tally");                                \
    for (unsigned int u_ = 0; u_ < 32; u_++) {                            \
        if (mtx_dbg_tally_ary[u_] > 0) {                                  \
            ctr_ += mtx_dbg_tally_ary[u_];                                \
            mtx_dbg_errseq (level, " %s:%d", label[u_],                   \
                            mtx_dbg_tally_ary[u_]);                       \
    }}                                                                    \
    mtx_dbg_errseq (level, " total:%lu\n", ctr_);                         \
} while (0)

double mtx_dbg_etime (unsigned);
const char *mtx_dbg_fmt_etime (double delta);

#else  /* MTX_DEBUG */

#define mtx_dbg_dprint(...)
#define mtx_dbg_errout(...)
#define mtx_dbg_errseq(...)
#define mtx_dbg_etime(...)
#define mtx_dbg_fmt_etime(...)
#define mtx_dbg_tally(...)
#define mtx_dbg_tally_reset(...)
#define mtx_dbg_print_tally(...)


#endif /* MTX_DEBUG */

/* usage: `mtx_dbg_err* (Z1_(cond, level), ...)` to skip printing if cond. */
#define Z1_(cond, level)         ((cond) ? -1 : (level))

#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif /* MTX_DBG_H */
