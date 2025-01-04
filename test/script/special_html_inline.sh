#!/bin/sh

TEST_LABEL='special_html_inline'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-smart --no-heading-link"
CMARK="$CMARK_BIN --nobreaks"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
