#!/bin/sh

TEST_LABEL='special_unsafe_html_textmode'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --unsafe-html --no-smart --no-ext --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
