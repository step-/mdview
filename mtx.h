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

#ifndef MTX_H
#define MTX_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum _MtxCmmWordType
{
    MTX_CMM_WORD_UNKNOWN = 0,     /* any word not of the following types */
    MTX_CMM_WORD_URI,             /* http:// ... */
    MTX_CMM_WORD_URI_FLANKED,     /* [<(]http://[)>] ... */
    MTX_CMM_WORD_ABS_PATH,        /* reasonably long /pathname */
    MTX_CMM_WORD_FILE_DIFF,       /* ".diff" ".patch" file name */
    MTX_CMM_WORD_BUGZILLA,        /* #asciiWord */
    MTX_CMM_WORD_FUNCNAME,        /* ident"()" */
    MTX_CMM_WORD_EMAIL,           /* @email address */
    MTX_CMM_WORD_UIDENT,          /* uppercase identifier with "_" */
} MtxCmmWordType;

MtxCmmWordType mtx_word_type (const gchar *, gint *, gint *);

G_END_DECLS

#endif /* MTX_H */
