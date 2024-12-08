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

#ifndef __MTX_STYLE_PANGO_H
#define __MTX_STYLE_PANGO_H

#include "mtxcolor.h"

/* Support for specifying font sizes in points or as percentages was added in Pango 1.50. */
#include <pango/pango-utils.h>
#if !PANGO_VERSION_CHECK(1,50,0)
#error "Pango version < 1.50.0 detected."
#endif

#define MTX_STYLE_PANGO_BLOCKQUOTE "fgcolor=\"silver\""
#define MTX_STYLE_PANGO_CODE_SPAN  "fgcolor=\""MTX_COLOR_CODE_SPAN_FG"\""
#define MTX_STYLE_PANGO_CODEBLOCK  "bgcolor=\""MTX_COLOR_CODEBLOCK_BG"\" fgcolor=\""MTX_COLOR_CODEBLOCK_FG"\""
#define MTX_STYLE_PANGO_H1         "fgcolor=\""MTX_COLOR_H1_FG"\" size=\"200%\""
#define MTX_STYLE_PANGO_H2         "fgcolor=\""MTX_COLOR_H2_FG"\" size=\"150%\""
#define MTX_STYLE_PANGO_H3         "fgcolor=\""MTX_COLOR_H3_FG"\" size=\"117%\""
#define MTX_STYLE_PANGO_H4         "fgcolor=\""MTX_COLOR_H4_FG"\" size=\"100%\" variant=\"smallcaps\""
#define MTX_STYLE_PANGO_H5         "fgcolor=\""MTX_COLOR_H5_FG"\" size=\"83%\""
#define MTX_STYLE_PANGO_H6         "fgcolor=\""MTX_COLOR_H6_FG"\" size=\"67%\""
#define MTX_STYLE_PANGO_IMAGE      "underline=\"double\""
#define MTX_STYLE_PANGO_URL        "fgcolor=\""MTX_COLOR_URL_FG"\" underline=\"single\""
#define MTX_STYLE_PANGO_TABLE      "bgcolor=\""MTX_COLOR_TABLE_BG"\" fgcolor=\""MTX_COLOR_TABLE_FG"\" size=\"108%\""
#define MTX_STYLE_PANGO_THEAD      "bgcolor=\""MTX_COLOR_THEAD_BG"\" fgcolor=\""MTX_COLOR_THEAD_FG"\""
#define MTX_STYLE_PANGO_TH         "bgcolor=\""MTX_COLOR_TH_BG"\" fgcolor=\""MTX_COLOR_TH_FG"\" weight=\"bold\""
#define MTX_STYLE_PANGO_TD         "bgcolor=\""MTX_COLOR_TD_BG"\" fgcolor=\""MTX_COLOR_TD_FG"\""

#endif /* __MTX_STYLE_PANGO_H */

