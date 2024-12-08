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

#ifndef MTX_CMM_PRIVATE_H
#define MTX_CMM_PRIVATE_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum _mtx_cmm_parser_unit_type
{
    MTX_CMM_PARSER_UNIT_NULL             = 0,

    /* Containers. */
    MTX_CMM_PARSER_UNIT_BLOCK_LI         = 1 << 0,
    MTX_CMM_PARSER_UNIT_BLOCK_P          = 1 << 1,

    MTX_CMM_PARSER_UNIT_BLOCK_QUOTE      = 1 << 2,
    MTX_CMM_PARSER_UNIT_BLOCK_CODE       = 1 << 3,
    MTX_CMM_PARSER_UNIT_BLOCK_HR         = 1 << 4,
    MTX_CMM_PARSER_UNIT_BLOCK_H          = 1 << 5,
    MTX_CMM_PARSER_UNIT_BLOCK_HTML       = 1 << 6,
    MTX_CMM_PARSER_UNIT_BLOCK_OL         = 1 << 7,
    MTX_CMM_PARSER_UNIT_BLOCK_UL         = 1 << 8,

    MTX_CMM_PARSER_UNIT_BLOCK_TABLE      = 1 << 9,
    MTX_CMM_PARSER_UNIT_BLOCK_THEAD      = 1 << 10,
    MTX_CMM_PARSER_UNIT_BLOCK_TBODY      = 1 << 11,
    MTX_CMM_PARSER_UNIT_BLOCK_TR         = 1 << 12,
    MTX_CMM_PARSER_UNIT_BLOCK_TH         = 1 << 13,
    MTX_CMM_PARSER_UNIT_BLOCK_TD         = 1 << 14,

    MTX_CMM_PARSER_UNIT_SPAN_A           = 1 << 15,
    MTX_CMM_PARSER_UNIT_SPAN_IMG         = 1 << 16,
    MTX_CMM_PARSER_UNIT_SPAN_CODE        = 1 << 17,

    MTX_CMM_PARSER_UNIT_RAW_HTML         = 1 << 18,
    MTX_CMM_PARSER_UNIT_ARG              = 1 << 19,
    MTX_CMM_PARSER_UNIT_ARG_INLINES      = 1 << 20,
    MTX_CMM_PARSER_UNIT_INLINES          = 1 << 21,
    MTX_CMM_PARSER_UNIT_JUNK             = 1 << 22,

} MtxCmmParserUnitType;

typedef enum _mtx_cmm_parser_unit_flag
{
    MTX_CMM_PARSER_UNIT_FLAG_NULL        = 0,
    MTX_CMM_PARSER_UNIT_FLAG_ARGS        = 1 << 0,
    MTX_CMM_PARSER_UNIT_FLAG_OPEN        = 1 << 1,
    MTX_CMM_PARSER_UNIT_FLAG_CLOSE       = 1 << 2,
} MtxCmmParserUnitFlag;

typedef struct _mtx_cmm_parser_unit
{
    MtxCmmParserUnitType                 type;
    MtxCmmParserUnitFlag                 flag;
    GString                              *text;
    GArray                               *args; /* (gchar *) */
} MtxCmmParserUnit;

typedef enum _MtxCmmRegexType
{
    MTX_CMM_REGEX_CODE_REF              = 0, /* internal code_refs */
    MTX_CMM_REGEX_DIRECTIVE,
    MTX_CMM_REGEX_WORD_SPLIT,
    MTX_CMM_REGEX_DUMB_QUOTE_PAIR,
    MTX_CMM_REGEX_UNIPUA,
    MTX_CMM_REGEX_TILDE_CODE_FENCE,

    /* keep last */
    MTX_CMM_REGEX_LEN,                       /* regex_table length */
} MtxCmmRegexType;

/*******************************************************************************
 ******************************************************************************/

#define MTX_MAX_LI_LEVEL 32 /* maximum OL/UL nesting depth */

/*******************************************************************************
 * The parser temporarily replaces tokens with Unicode Private Use Area (PUA)  *
 * codepoints. Input markdown shall not include the following PUA codepoints:  *
 ******************************************************************************/

/**
https://www.unicode.org/faq/private_use.html
https://github.com/silnrsi/unicode-resources/tree/main/sil-pua
https://utf8-chartable.de/unicode-utf8-table.pl?number=128&start=62976
*/

/* Markdown hard line-break (2+ spaces at the end of the line). */
#define iUNIPUA_BR         0xF600
#define sUNIPUA_BR         "\357\230\200"     /* 0xEF 0x98 0x80 */
#define rUNIPUA_BR         "\\x{F600}"
/* References for rendered code spans and sundries. */
#define iUNIPUA_CODE       0xF601
#define sUNIPUA_CODE       "\357\230\201"
#define rUNIPUA_CODE       "\\x{F601}"
/* References for links and image spans. */
#define iUNIPUA_LINK       0xF602
#define sUNIPUA_LINK       "\357\230\202"
#define rUNIPUA_LINK       "\\x{F602}"
/* <em> */
#define iUNIPUA_E1         0xF608
#define sUNIPUA_E1         "\357\230\210"
#define rUNIPUA_E1         "\\x{F608}"
/* </em> */
#define iUNIPUA_E0         0xF609
#define sUNIPUA_E0         "\357\230\211"
#define rUNIPUA_E0         "\\x{F609}"
/* <strong> */
#define iUNIPUA_B1         0xF60A
#define sUNIPUA_B1         "\357\230\212"
#define rUNIPUA_B1         "\\x{F60A}"
/* </strong> */
#define iUNIPUA_B0         0xF60B
#define sUNIPUA_B0         "\357\230\213"
#define rUNIPUA_B0         "\\x{F60B}"
/* &amp; */
#define iUNIPUA_AMP        0xF60C
#define sUNIPUA_AMP        "\357\230\214"
#define rUNIPUA_AMP        "\\x{F60C}"
/* &lt; */
#define iUNIPUA_LT         0xF60D
#define sUNIPUA_LT         "\357\230\215"
#define rUNIPUA_LT         "\\x{F60D}"
/* &gt; */
#define iUNIPUA_GT         0xF60E
#define sUNIPUA_GT         "\357\230\216"
#define rUNIPUA_GT         "\\x{F60E}"
/* &quot; */
#define iUNIPUA_QUOT       0xF60F
#define sUNIPUA_QUOT       "\357\230\217"
#define cUNIPUA_QUOT0      '\357'
#define cUNIPUA_QUOT1      '\230'
#define cUNIPUA_QUOT2      '\217'
#define rUNIPUA_QUOT       "\\x{F60F}"

/* reserved for public header */
/* #define iUNIPUA_PANGO_EMPTY_SPAN       0xF610 */

/*******************************************************************************
 ******************************************************************************/

G_END_DECLS

#endif /* MTX_CMM_PRIVATE_H */


