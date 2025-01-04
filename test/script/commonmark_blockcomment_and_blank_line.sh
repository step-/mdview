#!/bin/sh

TEST_LABEL='commonmark_blockcomment_and_blank_line'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
