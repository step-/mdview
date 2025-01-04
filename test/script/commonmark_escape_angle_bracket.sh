#!/bin/sh

TEST_LABEL='commonmark_escape_angle_bracket'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
