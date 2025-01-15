#!/bin/sh

TEST_LABEL='toc_code_block'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html pango'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --toc-level=6"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

