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

#ifndef __MTX_COLOR_H
#define __MTX_COLOR_H

/*
Pango color palette based on <https://personal.sron.nl/~pault/> 20231104.

Palette 1: Bright qualitative colour scheme that is colour-blind safe.
The main scheme for lines and their labels.
Blue, Red, Green, Yellow, Cyan, Purple, Gray
'#4477AA', '#EE6677', '#228833', '#CCBB44', '#66CCEE', '#AA3377', '#BBBBBB'

Palette 2: High-contrast qualitative colour scheme, an alternative to the bright
scheme of palette 1 that is colour-blind safe and optimized for contrast. [...]
It works well for people with monochrome vision and in a monochrome printout.
White, Blue, Yellow, Red, Black
'#FFFFFF','#004488', '#DDAA33', '#BB5566','#000000'

Palette 3: Vibrant qualitative colour scheme, an alternative to the bright
scheme of palette 1 that is equally colour-blind safe.
Orange, Blue,Cyan, Magenta, Red, Teal, Grey
'#EE7733', '#0077BB', '#33BBEE',
'#EE3377', '#CC3311', '#009988', '#BBBBBB'
*/

/**********************************************************************
*                           Using Palette #2                          *
***********************************************************************/

/*             Using palette #2             */
#define MTX_COLOR_BG           "#FFF"
#define MTX_COLOR_FG           "#000"
#define MTX_COLOR_CODE_SPAN_FG "#B56"
#define MTX_COLOR_CODEBLOCK_BG "#FFF" /* #EEE */
#define MTX_COLOR_CODEBLOCK_FG "#B56" /* #000 */
#define MTX_COLOR_H1_FG        "#444"
#define MTX_COLOR_H2_FG        "#444"
#define MTX_COLOR_H3_FG        "#444"
#define MTX_COLOR_H4_FG        "#000"
#define MTX_COLOR_H5_FG        "#000"
#define MTX_COLOR_H6_FG        "#000"
#define MTX_COLOR_URL_FG       "#048"
#define MTX_COLOR_HIGHLIGHT_BG "#FFF2B0"
/*#define MTX_COLOR_HIGHLIGHT_FG "#000"*/
#define MTX_COLOR_TABLE_BG     "#FBFBFB"
#define MTX_COLOR_TABLE_FG     "#EBEBEB"
#define MTX_COLOR_THEAD_BG     "#F4F4FF"
#define MTX_COLOR_THEAD_FG     "#E4E4E4"
#define MTX_COLOR_TH_BG        "#F4F4F4"
#define MTX_COLOR_TH_FG        "#222"
#define MTX_COLOR_TD_BG        "#FBFBFB"
#define MTX_COLOR_TD_FG        "#000"

#endif /* __MTX_COLOR_H */

