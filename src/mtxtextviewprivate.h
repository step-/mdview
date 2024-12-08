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


#ifndef MTX_TEXT_VIEW_PRIVATE_H
#define MTX_TEXT_VIEW_PRIVATE_H

G_BEGIN_DECLS

struct _MtxTextViewPrivateRendered
{
    gchar  *str;
    guint   len;     /* chars */
    guint   width;   /* pixel */
    guint   height;  /* pixel */
};

G_END_DECLS

#endif /* MTX_TEXT_VIEW_PRIVATE_H */
