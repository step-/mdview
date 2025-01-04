#!/bin/sh

TEST_LABEL='list_codeblock_2'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='ansi html pango text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

