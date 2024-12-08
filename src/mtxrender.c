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
/*
2024-08-15 step:
This file was derived from MD4C md4c_html.c with substantial changes and feature
additions for MDVIEW MTX.
*/
/****************/
/* *INDENT-OFF* */
/****************/
/*
 * MD4C: Markdown parser for C
 * (http://github.com/mity/md4c)
 *
 * Copyright (c) 2016-2024 Martin Mitáš
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "mtxrender.h"
#include "entity.h"
#include "mtxcmm.h"
#include "mtxcmmprivate.h"

void mtx_cmm_parser_unit_new (MtxCmm *, const MtxCmmParserUnitType, const MtxCmmParserUnitFlag);
int mtx_cmm_parser_find_unit_index (MtxCmm *, const MtxCmmParserUnitType, const MtxCmmParserUnitFlag, const int, MtxCmmParserUnit **);
gboolean mtx_cmm_parser_top_unit_ends_line (MtxCmm *);

#define PARSER(r)         ((MtxCmm *)(r)->userdata)

#define R2_FLUSH(r)        RENDER_VERBATIM(r, "")

#define R2_NEW_UNIT(r, type, flag_mask) \
    mtx_cmm_parser_unit_new (PARSER(r), type, flag_mask)

#define R2_ADD_ARG(r) \
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_ARG, MTX_CMM_PARSER_UNIT_FLAG_OPEN)

#define R2_SEAL_ARG(r) do { \
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_ARG, MTX_CMM_PARSER_UNIT_FLAG_CLOSE); \
    R2_FLUSH(r); \
} while (0)

#define R2_ADD_ARG_INLINES(r) \
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_ARG_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN)

#define R2_SEAL_ARG_INLINES(r) do { \
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_ARG_INLINES, MTX_CMM_PARSER_UNIT_FLAG_CLOSE); \
    R2_FLUSH(r); \
} while (0)

#define R2_GET_TOP_UNIT(r) mtx_cmm_parser_get_unit_head (PARSER(r))

#define R2_IS_TOP_UNIT(r, type_mask, flag_mask) \
    (mtx_cmm_parser_find_unit_index (PARSER(r), type_mask, flag_mask, 0, NULL) == 0)

#define R2_IS_UNIT_BELOW(r, type_mask, flag_mask) \
    (mtx_cmm_parser_find_unit_index (PARSER(r), type_mask, flag_mask, 1, NULL) == 1)

#define R2_TOP_UNIT_ENDS_WITH_NEWLINE(r) \
    mtx_cmm_parser_top_unit_ends_line (PARSER(r))

#define R2_SEAL_UNIT(r) R2_FLUSH(r)





#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199409L
    /* C89/90 or old compilers in general may not understand "inline". */
    #if defined __GNUC__
        #define inline __inline__
    #elif defined _MSC_VER
        #define inline __inline
    #else
        #define inline
    #endif
#endif

#ifdef _WIN32
    #define snprintf _snprintf
#endif



typedef struct MD_HTML_tag MD_HTML;
struct MD_HTML_tag {
    void (*process_output)(const MD_CHAR*, MD_SIZE, void*);
    void* userdata;
    unsigned flags;
    int image_nesting_level;
    char escape_map[256];
    int li_level;
    unsigned long li_is_tight; /* bit mask */
    int li_ordinal[MTX_MAX_LI_LEVEL];
    const MtxCmmTags* tags;
    gboolean output_html;
    gboolean escape;
    gboolean softbreak_tweak;
    gboolean html5_tweak;
    gboolean indent_li_block;
    gboolean inside_table;
};

#define NEED_HTML_ESC_FLAG   0x1
#define NEED_URL_ESC_FLAG    0x2


/*****************************************
 ***  HTML rendering helper functions  ***
 *****************************************/

#define ISDIGIT(ch)     ('0' <= (ch) && (ch) <= '9')
#define ISLOWER(ch)     ('a' <= (ch) && (ch) <= 'z')
#define ISUPPER(ch)     ('A' <= (ch) && (ch) <= 'Z')
#define ISALNUM(ch)     (ISLOWER(ch) || ISUPPER(ch) || ISDIGIT(ch))


static inline void
render_verbatim(MD_HTML* r, const MD_CHAR* text, MD_SIZE size)
{
    r->process_output(text, size, r->userdata);
}

/* Keep this as a macro. Most compiler should then be smart enough to replace
 * the strlen() call with a compile-time constant if the string is a C literal. */
#define RENDER_VERBATIM(r, verbatim)                                    \
        render_verbatim((r), (verbatim), (MD_SIZE) (strlen(verbatim)))


static void
render_html_escaped(MD_HTML* r, const MD_CHAR* data, MD_SIZE size)
{
    MD_OFFSET beg = 0;
    MD_OFFSET off = 0;

    /* Some characters need to be escaped in normal HTML text. */
    #define NEED_HTML_ESC(ch)   (r->escape_map[(unsigned char)(ch)] & NEED_HTML_ESC_FLAG)

    while(1) {
        /* Optimization: Use some loop unrolling. */
        while(off + 3 < size  &&  !NEED_HTML_ESC(data[off+0])  &&  !NEED_HTML_ESC(data[off+1])
                              &&  !NEED_HTML_ESC(data[off+2])  &&  !NEED_HTML_ESC(data[off+3]))
            off += 4;
        while(off < size  &&  !NEED_HTML_ESC(data[off]))
            off++;

        if(off > beg)
            render_verbatim(r, data + beg, off - beg);

        if(off < size) {
            switch(data[off]) {
                case '&':   RENDER_VERBATIM(r, sUNIPUA_AMP); break;
                case '<':   RENDER_VERBATIM(r, sUNIPUA_LT); break;
                case '>':   RENDER_VERBATIM(r, sUNIPUA_GT); break;
                case '"':   RENDER_VERBATIM(r, sUNIPUA_QUOT); break;
            }
            off++;
        } else {
            break;
        }
        beg = off;
    }
}

static void
render_url_escaped(MD_HTML* r, const MD_CHAR* data, MD_SIZE size)
{
    static const MD_CHAR hex_chars[] = "0123456789ABCDEF";
    MD_OFFSET beg = 0;
    MD_OFFSET off = 0;

    /* Some characters need to be escaped in URL attributes. */
    #define NEED_URL_ESC(ch)    (r->escape_map[(unsigned char)(ch)] & NEED_URL_ESC_FLAG)

    while(1) {
        while(off < size  &&  !NEED_URL_ESC(data[off]))
            off++;
        if(off > beg)
            render_verbatim(r, data + beg, off - beg);

        if(off < size) {
            char hex[3];

            switch(data[off]) {
                case '&':   RENDER_VERBATIM(r, "&amp;"); break;
                default:
                    hex[0] = '%';
                    hex[1] = hex_chars[((unsigned)data[off] >> 4) & 0xf];
                    hex[2] = hex_chars[((unsigned)data[off] >> 0) & 0xf];
                    render_verbatim(r, hex, 3);
                    break;
            }
            off++;
        } else {
            break;
        }

        beg = off;
    }
}

static unsigned
hex_val(char ch)
{
    if('0' <= ch && ch <= '9')
        return ch - '0';
    if('A' <= ch && ch <= 'Z')
        return ch - 'A' + 10;
    else
        return ch - 'a' + 10;
}

static void
render_utf8_codepoint(MD_HTML* r, unsigned codepoint,
                      void (*fn_append)(MD_HTML*, const MD_CHAR*, MD_SIZE))
{
    static const MD_CHAR utf8_replacement_char[] = { (char)0xef, (char)0xbf, (char)0xbd };

    unsigned char utf8[4];
    size_t n;

    if(codepoint <= 0x7f) {
        n = 1;
        utf8[0] = codepoint;
    } else if(codepoint <= 0x7ff) {
        n = 2;
        utf8[0] = 0xc0 | ((codepoint >>  6) & 0x1f);
        utf8[1] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else if(codepoint <= 0xffff) {
        n = 3;
        utf8[0] = 0xe0 | ((codepoint >> 12) & 0xf);
        utf8[1] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else {
        n = 4;
        utf8[0] = 0xf0 | ((codepoint >> 18) & 0x7);
        utf8[1] = 0x80 + ((codepoint >> 12) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[3] = 0x80 + ((codepoint >>  0) & 0x3f);
    }

    if(0 < codepoint  &&  codepoint <= 0x10ffff)
        fn_append(r, (char*)utf8, (MD_SIZE)n);
    else
        fn_append(r, utf8_replacement_char, 3);
}

/* Translate entity to its UTF-8 equivalent, or output the verbatim one
 * if such entity is unknown (or if the translation is disabled). */
static void
render_entity(MD_HTML* r, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MD_HTML*, const MD_CHAR*, MD_SIZE))
{
    if(r->flags & MD_HTML_FLAG_VERBATIM_ENTITIES) {
        render_verbatim(r, text, size);
        return;
    }

    /* We assume UTF-8 output is what is desired. */
    if(size > 3 && text[1] == '#') {
        unsigned codepoint = 0;

        if(text[2] == 'x' || text[2] == 'X') {
            /* Hexadecimal entity (e.g. "&#x1234abcd;")). */
            MD_SIZE i;
            for(i = 3; i < size-1; i++)
                codepoint = 16 * codepoint + hex_val(text[i]);
        } else {
            /* Decimal entity (e.g. "&1234;") */
            MD_SIZE i;
            for(i = 2; i < size-1; i++)
                codepoint = 10 * codepoint + (text[i] - '0');
        }

        render_utf8_codepoint(r, codepoint, fn_append);
        return;
    } else {
        /* Named entity (e.g. "&nbsp;"). */
        const ENTITY* ent;

        ent = entity_lookup(text, size);
        if(ent != NULL) {
            render_utf8_codepoint(r, ent->codepoints[0], fn_append);
            if(ent->codepoints[1])
                render_utf8_codepoint(r, ent->codepoints[1], fn_append);
            return;
        }
    }

    fn_append(r, text, size);
}

static void
render_attribute(MD_HTML* r, const MD_ATTRIBUTE* attr,
                 void (*fn_append)(MD_HTML*, const MD_CHAR*, MD_SIZE))
{
    int i;

    for(i = 0; attr->substr_offsets[i] < attr->size; i++) {
        MD_TEXTTYPE type = attr->substr_types[i];
        MD_OFFSET off = attr->substr_offsets[i];
        MD_SIZE size = attr->substr_offsets[i+1] - off;
        const MD_CHAR* text = attr->text + off;

        switch(type) {
            case MD_TEXT_NULLCHAR:  render_utf8_codepoint(r, 0x0000, render_verbatim); break;
            case MD_TEXT_ENTITY:    render_entity(r, text, size, fn_append); break;
            default:                fn_append(r, text, size); break;
        }
    }
}

static void
render_open_blockquote_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_QUOTE, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    RENDER_VERBATIM(r, r->tags->blockquote_start);
}

static void
render_close_blockquote_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_QUOTE, MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM(r, r->tags->blockquote_end);
    R2_SEAL_UNIT(r);
}

static void
render_intro_open_ol_ul_block(MD_HTML* r, const void* det, const MtxCmmParserUnitType type)
{
    int is_tight, start = -1, add_newline = 1;

    if (type == MTX_CMM_PARSER_UNIT_BLOCK_OL) {
        is_tight = ((MD_BLOCK_OL_DETAIL*)det)->is_tight;
        start    = ((MD_BLOCK_OL_DETAIL*)det)->start;
    }
    else {
        is_tight = ((MD_BLOCK_UL_DETAIL*)det)->is_tight;
    }

    r->li_level++;
    if (is_tight)
        r->li_is_tight |= (1L << r->li_level);
    r->li_ordinal[r->li_level % MTX_MAX_LI_LEVEL] = start;

    add_newline = !R2_TOP_UNIT_ENDS_WITH_NEWLINE(r);

    R2_NEW_UNIT(r, type, MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    if (add_newline && r->li_level > 0)
        RENDER_VERBATIM(r, "\n");
}

static inline void
render_outtro_open_ol_ul_block(MD_HTML* r)
{
    /* clear level */
    r->li_is_tight &= ~(1UL << r->li_level);
    r->li_level--;
}

static void
render_open_ul_block(MD_HTML* r, const MD_BLOCK_UL_DETAIL* det)
{
    render_intro_open_ol_ul_block(r, det, MTX_CMM_PARSER_UNIT_BLOCK_UL);
    RENDER_VERBATIM(r, r->tags->ulist_start);
}

static void
render_close_ul_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_UL, MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM(r, r->tags->ulist_end);
    R2_SEAL_UNIT(r);
    render_outtro_open_ol_ul_block(r);
}

static void
render_open_ol_block(MD_HTML* r, const MD_BLOCK_OL_DETAIL* det)
{
    char buf[64];
    int len;

    render_intro_open_ol_ul_block(r, det, MTX_CMM_PARSER_UNIT_BLOCK_OL);
    if(!r->output_html) {
        RENDER_VERBATIM(r, r->tags->olist_start);
        return;
    }

    if(det->start == 1) {
        RENDER_VERBATIM(r, "<ol>\n");
        return;
    }

    len = snprintf(buf, sizeof(buf), "<ol start=\"%u\">\n", det->start);
    render_verbatim(r, buf, len);
}

static void
render_close_ol_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_OL, MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM(r, r->tags->olist_end);
    R2_SEAL_UNIT(r);
    render_outtro_open_ol_ul_block(r);
}

static void
render_open_li_block(MD_HTML* r, const MD_BLOCK_LI_DETAIL* det __attribute__((unused)))
{
    int ordinal  = r->li_ordinal[r->li_level];
    int len;
    char arg[16];

    if (ordinal >= 0)
        ++r->li_ordinal[r->li_level];

    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_LI, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    R2_ADD_ARG(r);
    if(r->output_html) {

#if 1
        RENDER_VERBATIM(r, r->tags->li_start[r->li_level % 2]);

#else /* TODO (likely never) */
    if(det->is_task) {
        RENDER_VERBATIM(r, "<li class=\"task-list-item\">"
                          "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled");
        if(det->task_mark == 'x' || det->task_mark == 'X')
            RENDER_VERBATIM(r, " checked");
        RENDER_VERBATIM(r, ">");
    } else {
        RENDER_VERBATIM(r, "<li>");
    }
#endif
    }
    else {
        char buf[MTX_MAX_LI_LEVEL * 2] = /* 63 spaces (indentation) */
        "                                                               ";
        int ind_len = 2 * (1 + r->li_level % (sizeof(buf) / 2));
        buf[ind_len] = '\0';

        /* Render indentation. */
        if (r->indent_li_block)
            render_verbatim(r, buf, ind_len);

        /* Render start tag for ol or ul. */
        if (ordinal >= 0)
            len = snprintf (buf, sizeof(buf), "%d. ", ordinal); /* ol */
        else
            len = snprintf (buf, sizeof(buf), "%s ", r->tags->li_start[r->li_level % 2]); /* ul */
        render_verbatim(r, buf, len);
    }
    R2_SEAL_ARG(r);

    R2_ADD_ARG(r);
    render_verbatim(r, r->li_is_tight & (1L << r->li_level) ? "T" : "F", 1);
    R2_SEAL_ARG(r);

    len = snprintf(arg, sizeof(arg), "%d", r->li_level + 1);
    R2_ADD_ARG(r);
    render_verbatim(r, arg, len);
    R2_SEAL_ARG(r);

    len = snprintf(arg, sizeof(arg), "%d", ordinal);
    R2_ADD_ARG(r);
    render_verbatim(r, arg, len);
    R2_SEAL_ARG(r);

    /* FIXME li can contain blockquote => arbitrary blocks! */

    /* For units to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static void
render_close_li_block(MD_HTML* r)
{
    /*
    For non-HTML output modes, collapse runs of bullet endings to preserve
    vertical space.  Consider the following cases:
    - "</p></li>" -- loose lists
    - "</li></li>", "</ol></li>" and "</ul></li>" -- nested lists
    */
    int collapse_li_ends =
    R2_IS_TOP_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_P | MTX_CMM_PARSER_UNIT_BLOCK_LI | MTX_CMM_PARSER_UNIT_BLOCK_OL |
        MTX_CMM_PARSER_UNIT_BLOCK_UL, MTX_CMM_PARSER_UNIT_FLAG_CLOSE);

    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_LI, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_CLOSE);

    R2_ADD_ARG(r);
    if (r->output_html) {
        RENDER_VERBATIM(r, r->tags->li_end);
    }
    else {
        if (!collapse_li_ends) {
            RENDER_VERBATIM(r, r->tags->li_end);
        }
    }
    R2_SEAL_ARG(r);
    R2_SEAL_UNIT(r);
}

static void
render_open_hr_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_HR, MTX_CMM_PARSER_UNIT_FLAG_OPEN | MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM(r, r->tags->rule);
    if (r->output_html) {
        RENDER_VERBATIM(r, r->html5_tweak ? ">\n" : " />\n");
    }
    R2_SEAL_UNIT(r);
}

static void
render_open_h_block(MD_HTML* r, const MD_BLOCK_H_DETAIL* det)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_H, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    R2_ADD_ARG(r);
    switch (det->level) {
        case 1: RENDER_VERBATIM(r, r->tags->h1_start); break;
        case 2: RENDER_VERBATIM(r, r->tags->h2_start); break;
        case 3: RENDER_VERBATIM(r, r->tags->h3_start); break;
        case 4: RENDER_VERBATIM(r, r->tags->h4_start); break;
        case 5: RENDER_VERBATIM(r, r->tags->h5_start); break;
        case 6: RENDER_VERBATIM(r, r->tags->h6_start); break;
    }
    R2_SEAL_ARG(r);
    R2_SEAL_UNIT(r);

    /* For inlines to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static void
render_close_h_block(MD_HTML* r, const MD_BLOCK_H_DETAIL* det)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_H, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    R2_ADD_ARG(r);
    switch (det->level) {
        case 1: RENDER_VERBATIM(r, r->tags->h1_end); break;
        case 2: RENDER_VERBATIM(r, r->tags->h2_end); break;
        case 3: RENDER_VERBATIM(r, r->tags->h3_end); break;
        case 4: RENDER_VERBATIM(r, r->tags->h4_end); break;
        case 5: RENDER_VERBATIM(r, r->tags->h5_end); break;
        case 6: RENDER_VERBATIM(r, r->tags->h6_end); break;
    }
    R2_SEAL_ARG(r);
    R2_SEAL_UNIT(r);
}

static void
render_open_code_block(MD_HTML* r, const MD_BLOCK_CODE_DETAIL* det)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_CODE, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    if (r->output_html) {

        RENDER_VERBATIM(r, r->tags->codeblock_start);

        /* If known, output the HTML 5 attribute class="language-LANGNAME". */
        if(det->lang.text != NULL) {
            RENDER_VERBATIM(r, " class=\"language-");
            render_attribute(r, &det->lang, render_html_escaped);
            RENDER_VERBATIM(r, "\"");
        }

        RENDER_VERBATIM(r, ">");
    }
    else
        RENDER_VERBATIM(r, r->tags->codeblock_start);
}

static void
render_close_code_block(MD_HTML* r, const MD_BLOCK_CODE_DETAIL* det __attribute__((unused)))
{
    /*
    Pass end tag as the argument of a new unit to allow mtx_cmm_mtx to access
    the text field of the opening unit without having to pry it open.
    */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_CODE, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    R2_ADD_ARG(r);
    RENDER_VERBATIM(r, r->tags->codeblock_end);
    R2_SEAL_ARG(r);
}

static inline void
render_open_html_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_HTML, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static inline void
render_close_html_block(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_HTML, MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
}

static void
render_open_p_block (MD_HTML *r)
{
    /* Imitate cmark's output in loose list. */
    if (r->output_html && R2_IS_UNIT_BELOW(r, MTX_CMM_PARSER_UNIT_BLOCK_LI, MTX_CMM_PARSER_UNIT_FLAG_OPEN))
        RENDER_VERBATIM(r, "\n");

    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_P, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    R2_ADD_ARG(r);
    RENDER_VERBATIM (r, r->tags->para_start);
    R2_SEAL_ARG(r);

    /* For inlines to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static void
render_close_p_block (MD_HTML *r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_P, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_CLOSE);

    R2_ADD_ARG(r);
    if (r->output_html) {
        RENDER_VERBATIM(r, r->tags->para_end);
    }
    else {
        RENDER_VERBATIM(r, r->tags->para_end);
    }
    R2_SEAL_ARG(r);
    R2_SEAL_UNIT(r);
}

static void
render_open_table_block(MD_HTML* r)
{
    r->inside_table = 1;
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TABLE,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    if (r->output_html) {
        RENDER_VERBATIM (r, r->tags->table_start);
    } else {
        /* Placeholder for serialized max_col_width array */
        R2_ADD_ARG(r);
        RENDER_VERBATIM (r, "");
        R2_SEAL_ARG(r);

        RENDER_VERBATIM (r, r->tags->table_start);
    }
    R2_SEAL_UNIT(r);
}

static void
render_close_table_block(MD_HTML* r)
{
    r->inside_table = 0;
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TABLE,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM (r, r->tags->table_end);
    R2_SEAL_UNIT(r);
}

static void
render_open_thead_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_THEAD,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    RENDER_VERBATIM (r, r->tags->thead_start);
    R2_SEAL_UNIT(r);
}

static void
render_close_thead_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_THEAD,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM (r, r->tags->thead_end);
    R2_SEAL_UNIT(r);
}

static void
render_open_tbody_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TBODY,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    if (r->output_html) {
        RENDER_VERBATIM (r, r->tags->tbody_start);
    }
    else {
        R2_ADD_ARG(r);
        RENDER_VERBATIM (r, r->tags->tbody_start);
        R2_SEAL_ARG(r);
    }

    R2_SEAL_UNIT(r);
}

static void
render_close_tbody_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TBODY,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_CLOSE);

    if (r->output_html) {
        RENDER_VERBATIM (r, r->tags->tbody_end);
    }
    else {
        R2_ADD_ARG(r);
        RENDER_VERBATIM (r, r->tags->tbody_end);
        R2_SEAL_ARG(r);
    }

    R2_SEAL_UNIT(r);
}

static void
render_open_tr_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TR,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    RENDER_VERBATIM (r, r->tags->tr_start);
    R2_SEAL_UNIT(r);
}

static void
render_close_tr_block(MD_HTML* r)
{
    R2_NEW_UNIT (r, MTX_CMM_PARSER_UNIT_BLOCK_TR,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM (r, r->tags->tr_end);
    R2_SEAL_UNIT(r);
}

static void
render_open_td_block(MD_HTML* r, const int cell_type, const MD_BLOCK_TD_DETAIL* det)
{
    const char *align;
    int len;
    char buf[32];

    R2_NEW_UNIT (r, cell_type == 0 ? MTX_CMM_PARSER_UNIT_BLOCK_TH :
                 MTX_CMM_PARSER_UNIT_BLOCK_TD,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    if (r->output_html) {
        switch (det->align) {
        case MD_ALIGN_LEFT:
            align = " align=\"left\"";
            break;
        case MD_ALIGN_CENTER:
            align = " align=\"center\"";
            break;
        case MD_ALIGN_RIGHT:
            align = " align=\"right\"";
            break;
        default:
            align = "";
            break;
        }
        len = snprintf (buf, sizeof buf, cell_type == 0 ? r->tags->th_start :
                        r->tags->td_start, align);
        render_verbatim (r, buf, len);
    }
    else {
        /* Prefix cell text with its alignment encoded as an ASCII character.
        This avoids having empty cells, which sink before reaching the queue. */
        buf[0] = "NLCR"[det->align]; /* (N)one L(eft) C(enter) R(ight) */
        render_verbatim (r, buf, 1);

        R2_ADD_ARG (r);
        RENDER_VERBATIM (r,
                         cell_type ==
                         0 ? r->tags->th_start : r->tags->td_start);
        R2_SEAL_ARG (r);
    }

    R2_SEAL_UNIT (r);
}

static void
render_close_td_block(MD_HTML* r, const int cell_type)
{
    R2_NEW_UNIT (r, cell_type == 0 ? MTX_CMM_PARSER_UNIT_BLOCK_TH :
                 MTX_CMM_PARSER_UNIT_BLOCK_TD,
                 MTX_CMM_PARSER_UNIT_FLAG_ARGS |
                 MTX_CMM_PARSER_UNIT_FLAG_CLOSE);
    RENDER_VERBATIM (r, cell_type == 0 ? r->tags->th_end : r->tags->td_end);
    R2_SEAL_UNIT (r);
}

static void
render_open_a_span(MD_HTML* r, const MD_SPAN_A_DETAIL* det)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_SPAN_A, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    /* Provide choice of link format: URI-encoded and verbatim.
    */
    R2_ADD_ARG(r);
    render_attribute (r, &det->href, render_url_escaped);
    RENDER_VERBATIM(r, "\n"); /*value separator*/
    render_attribute (r, &det->href, render_verbatim);
    R2_SEAL_ARG(r);

    R2_ADD_ARG(r);
    if (det->title.text)
      render_attribute(r, &det->title, r->escape ? render_html_escaped : render_verbatim);
    R2_SEAL_ARG(r);

    /* Upcoming arg is this span's text. */
    R2_ADD_ARG_INLINES(r);

    /* Inject a prefix to the span's text. With this, the rendering coda will be
    able to compensate <table> column alignment for <a> links. */
    render_verbatim (r, r->inside_table ? "1" : "0", 1);
}

static void
render_close_a_span(MD_HTML* r, const MD_SPAN_A_DETAIL* det __attribute__((unused)))
{
    /* After text_callback. */
    R2_SEAL_ARG_INLINES(r);

    /* With URL, title and link text in head unit's args. */

    /* For inlines to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static void
render_open_img_span(MD_HTML* r, const MD_SPAN_IMG_DETAIL* det)
{
    /*
    When nested images are involved, render_open_img_span is called only once:
    for the outer image with image_nesting_level == 1.
    */
    g_assert (r->image_nesting_level == 1);

    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_SPAN_IMG, MTX_CMM_PARSER_UNIT_FLAG_ARGS | MTX_CMM_PARSER_UNIT_FLAG_OPEN);

    /* Provide choice of path format: URI-encoded and verbatim.
    */
    R2_ADD_ARG(r);
    render_attribute (r, &det->src, render_url_escaped);
    RENDER_VERBATIM(r, "\n"); /*value separator*/
    render_attribute (r, &det->src, render_verbatim);
    R2_SEAL_ARG(r);

    R2_ADD_ARG(r);
    if(det->title.text)
        render_attribute(r, &det->title, render_verbatim);
    R2_SEAL_ARG(r);

    /* Upcoming arg is this span's text. */
    R2_ADD_ARG(r);

    /*
    For nested images, this is the outer image and the above ARG will stay open
    to conflate render text() calls until the outer image will be closed in
    render_close_img_span.
    */

    /* Inject a prefix to the span's text. With this, the rendering coda will be
    able to compensate <table> column alignment for <a> links. */
    render_verbatim (r, r->inside_table ? "1" : "0", 1);
}

static void
render_close_img_span(MD_HTML* r, const MD_SPAN_IMG_DETAIL* det __attribute__((unused)))
{
    /* Ditto as in render_open_img_span but image_nesting_level == 0. */
    g_assert (r->image_nesting_level == 0);

    /* After text_callback. */
    R2_SEAL_ARG(r);

    /* With path, title and alt text in head unit's args. */

    /* For inlines to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

static void
render_open_code_span(MD_HTML* r)
{
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_SPAN_CODE, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    RENDER_VERBATIM(r, r->tags->code_span_start);
}

static void
render_close_code_span(MD_HTML* r)
{
    /* after text_callback */

    /* With formatted code in head unit's text. */

    RENDER_VERBATIM(r, r->tags->code_span_end);

    /* For inlines to come (auto-closing). */
    R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
}

#if 0 /*TODO future*/
static void
render_open_wikilink_span(MD_HTML* r, const MD_SPAN_WIKILINK_DETAIL* det)
{
    RENDER_VERBATIM(r, "<x-wikilink data-target=\"");
    render_attribute(r, &det->target, render_html_escaped);

    RENDER_VERBATIM(r, "\">");
}
#endif

static void
render_text_hardbr(MD_HTML* r)
{
    /* image_nesting_level == 0 means this text is outside an image span. */
    RENDER_VERBATIM(r, (r->image_nesting_level == 0 ? sUNIPUA_BR : " "));
}

static void
render_text_softbr(MD_HTML* r)
{
    /* image_nesting_level == 0 means this text is outside an image span. */
    RENDER_VERBATIM (r, (r->softbreak_tweak && r->image_nesting_level == 0) ?
                     "\n" : " ");
}

static inline void
render_text_html(MD_HTML* r, const MD_CHAR* text, const MD_SIZE size)
{
    if (R2_IS_TOP_UNIT(r, MTX_CMM_PARSER_UNIT_BLOCK_HTML, MTX_CMM_PARSER_UNIT_FLAG_OPEN))
        /* Line inside a larger HTML block. */
        render_verbatim(r, text, size);
    else {
        /* Inline HTML tags. */
        int is_inline = R2_IS_TOP_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);

        R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_RAW_HTML, 0);
        render_verbatim(r, text, size);

        if (is_inline)
            R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_INLINES, MTX_CMM_PARSER_UNIT_FLAG_OPEN);
    }
}

/**************************************
 ***  HTML renderer implementation  ***
 **************************************/

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;

    switch(type) {
        case MD_BLOCK_DOC:      R2_NEW_UNIT(r, MTX_CMM_PARSER_UNIT_NULL, MTX_CMM_PARSER_UNIT_FLAG_OPEN); break;
        case MD_BLOCK_QUOTE:    render_open_blockquote_block(r); break;
        case MD_BLOCK_UL:       render_open_ul_block(r, (const MD_BLOCK_UL_DETAIL*)detail); break;
        case MD_BLOCK_OL:       render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail); break;
        case MD_BLOCK_LI:       render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail); break;
        case MD_BLOCK_HR:       render_open_hr_block(r); break;
        case MD_BLOCK_H:        render_open_h_block(r, (const MD_BLOCK_H_DETAIL*)detail); break;
        case MD_BLOCK_CODE:     render_open_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail); break;
        case MD_BLOCK_HTML:     if(r->output_html) render_open_html_block(r); break;
        case MD_BLOCK_P:        render_open_p_block(r); break;
        default: ;              /* hush -Wswitch warning */
        case MD_BLOCK_TABLE:    render_open_table_block(r); break;
        case MD_BLOCK_THEAD:    render_open_thead_block(r); break;
        case MD_BLOCK_TBODY:    render_open_tbody_block(r); break;
        case MD_BLOCK_TR:       render_open_tr_block(r); break;
        case MD_BLOCK_TH:       render_open_td_block(r, 0, (MD_BLOCK_TD_DETAIL*)detail); break;
        case MD_BLOCK_TD:       render_open_td_block(r, 1, (MD_BLOCK_TD_DETAIL*)detail); break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;

    switch(type) {
        case MD_BLOCK_DOC:      R2_SEAL_UNIT(r); break;
        case MD_BLOCK_QUOTE:    render_close_blockquote_block(r); break;
        case MD_BLOCK_UL:       render_close_ul_block(r); break;
        case MD_BLOCK_OL:       render_close_ol_block(r); break;
        case MD_BLOCK_LI:       render_close_li_block(r); break;
        case MD_BLOCK_HR:       /*noop*/ break;
        case MD_BLOCK_H:        render_close_h_block(r, (const MD_BLOCK_H_DETAIL*)detail); break;
        case MD_BLOCK_CODE:     render_close_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail); break;
        case MD_BLOCK_HTML:     if(r->output_html) render_close_html_block(r); break;
        case MD_BLOCK_P:        render_close_p_block(r); break;
        default: ;              /* hush -Wswitch warning */
        case MD_BLOCK_TABLE:    render_close_table_block(r); break;
        case MD_BLOCK_THEAD:    render_close_thead_block(r); break;
        case MD_BLOCK_TBODY:    render_close_tbody_block(r); break;
        case MD_BLOCK_TR:       render_close_tr_block(r); break;
        case MD_BLOCK_TH:       render_close_td_block(r, 0); break;
        case MD_BLOCK_TD:       render_close_td_block(r, 1); break;
    }

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;
    int inside_img = (r->image_nesting_level > 0);

    /* We are inside a Markdown image label. Markdown allows to use any emphasis
     * and other rich contents in that context similarly as in any link label.
     *
     * However, unlike in the case of links (where that contents becomescontents
     * of the <a>...</a> tag), in the case of images the contents is supposed to
     * fall into the attribute alt: <img alt="...">.
     *
     * In that context we naturally cannot output nested HTML tags. So lets
     * suppress them and only output the plain text (i.e. what falls into text()
     * callback).
     *
     * CommonMark specification declares this a recommended practice for HTML
     * output.
     */
    if(type == MD_SPAN_IMG)
        r->image_nesting_level++;
    if(inside_img)
        return 0;

    switch(type) {
        case MD_SPAN_EM:                RENDER_VERBATIM(r, sUNIPUA_E1); break;
        case MD_SPAN_STRONG:            RENDER_VERBATIM(r, sUNIPUA_B1); break;
        case MD_SPAN_U:                 RENDER_VERBATIM(r, "<u>"); break;
        case MD_SPAN_A:                 render_open_a_span(r, (MD_SPAN_A_DETAIL*) detail); break;
        case MD_SPAN_IMG:               render_open_img_span(r, (MD_SPAN_IMG_DETAIL*) detail); break;
        case MD_SPAN_CODE:              render_open_code_span(r); break;
        case MD_SPAN_DEL:               RENDER_VERBATIM(r, r->tags->strikethrough_start); break;
        default: ;              /* hush -Wswitch warning */
#if 0 /*TODO future*/
        case MD_SPAN_LATEXMATH:         RENDER_VERBATIM(r, "<x-equation>"); break;
        case MD_SPAN_LATEXMATH_DISPLAY: RENDER_VERBATIM(r, "<x-equation type=\"display\">"); break;
        case MD_SPAN_WIKILINK:          render_open_wikilink_span(r, (MD_SPAN_WIKILINK_DETAIL*) detail); break;
#endif
    }

    return 0;
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;

    if(type == MD_SPAN_IMG)
        r->image_nesting_level--;
    if(r->image_nesting_level > 0)
        return 0;

    switch(type) {
        case MD_SPAN_EM:                RENDER_VERBATIM(r, sUNIPUA_E0); break;
        case MD_SPAN_STRONG:            RENDER_VERBATIM(r, sUNIPUA_B0); break;
        case MD_SPAN_U:                 RENDER_VERBATIM(r, "</u>"); break;
        case MD_SPAN_A:                 render_close_a_span(r, (MD_SPAN_A_DETAIL*) detail); break;
        case MD_SPAN_IMG:               render_close_img_span(r, (MD_SPAN_IMG_DETAIL*) detail); break;
        case MD_SPAN_CODE:              render_close_code_span(r); break;
        case MD_SPAN_DEL:               RENDER_VERBATIM(r, r->tags->strikethrough_end); break;
        default: ;              /* hush -Wswitch warning */
#if 0 /*TODO future*/
        case MD_SPAN_LATEXMATH:         /*fall through*/
        case MD_SPAN_LATEXMATH_DISPLAY: RENDER_VERBATIM(r, "</x-equation>"); break;
        case MD_SPAN_WIKILINK:          RENDER_VERBATIM(r, "</x-wikilink>"); break;
#endif
    }

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;

    switch(type) {
        case MD_TEXT_NULLCHAR:  render_utf8_codepoint(r, 0x0000, render_verbatim); break;
        case MD_TEXT_BR:        render_text_hardbr(r);
                                break;
        case MD_TEXT_SOFTBR:    render_text_softbr(r); break;
        case MD_TEXT_HTML:      if(r->output_html) render_text_html(r, text, size); break;
        case MD_TEXT_ENTITY:    render_entity(r, text, size, render_html_escaped); break;
        default:                if(r->escape) render_html_escaped(r, text, size);
                                else                render_verbatim(r, text, size);
                                break;
    }

    return 0;
}

static void
debug_log_callback(const char* msg, void* userdata)
{
    MD_HTML* r = (MD_HTML*) userdata;
    if(r->flags & MD_HTML_FLAG_DEBUG)
        fprintf(stderr, "MD4C: %s\n", msg);
}

int
md_mtx (const MD_CHAR* input, MD_SIZE input_size,
        void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
        void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    MD_HTML render = { process_output, userdata, renderer_flags, 0, { 0 },
        -1, 0, { 0 }, 0, 0, 0, 0, 0, 0, 0,
    };
    int i;

    MD_PARSER parser = {
        0,
        parser_flags,
        enter_block_callback,
        leave_block_callback,
        enter_span_callback,
        leave_span_callback,
        text_callback,
        debug_log_callback,
        NULL
    };

    /* Build map of characters which need escaping. */
    for(i = 0; i < 256; i++) {
        unsigned char ch = (unsigned char) i;

        if(strchr("\"&<>", ch) != NULL)
            render.escape_map[i] |= NEED_HTML_ESC_FLAG;

        if(!ISALNUM(ch)  &&  strchr("~-_.+!*(),%#@?=;:/,+$", ch) == NULL)
            render.escape_map[i] |= NEED_URL_ESC_FLAG;
    }

    /* Consider skipping UTF-8 byte order mark (BOM). */
    if(renderer_flags & MD_HTML_FLAG_SKIP_UTF8_BOM  &&  sizeof(MD_CHAR) == 1) {
        static const MD_CHAR bom[3] = { (char)0xef, (char)0xbb, (char)0xbf };
        if(input_size >= sizeof(bom)  &&  memcmp(input, bom, sizeof(bom)) == 0) {
            input += sizeof(bom);
            input_size -= sizeof(bom);
        }
    }

    g_assert (render.userdata);
    render.tags = mtx_cmm_get_output_tags (render.userdata);
    render.output_html = mtx_cmm_get_output (render.userdata) == MTX_CMM_OUTPUT_HTML;
    render.escape = render.output_html || mtx_cmm_get_escape (render.userdata);
    render.softbreak_tweak = mtx_cmm_get_tweaks (render.userdata) & MTX_CMM_TWEAK_SOFT_BREAK;
    render.html5_tweak = mtx_cmm_get_tweaks (render.userdata) & MTX_CMM_TWEAK_HTML5;
    render.indent_li_block = mtx_cmm_get_render_indent (render.userdata);

    return md_parse(input, input_size, &parser, (void*) &render);
}

