#!/bin/sh

TEST_LABEL='special_invalid_tag_a_name'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --unsafe-html --no-smart --no-ext --no-heading-link"
CMARK="$CMARK_BIN --unsafe"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
