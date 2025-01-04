#!/bin/sh

TEST_LABEL='special_html_inline_unsafe'
TEST_ARGS="test/subject/special_html_inline.md" # [sic]
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --unsafe-html --no-smart --no-heading-link"
CMARK="$CMARK_BIN --unsafe --nobreaks"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
