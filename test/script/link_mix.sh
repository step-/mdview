#!/bin/sh

TEST_LABEL='link_mix'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='ansi html pango text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

