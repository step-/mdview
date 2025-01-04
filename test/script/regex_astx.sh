#!/bin/sh

TEST_LABEL='regex_astx'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html pango text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --toc-level=6"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

