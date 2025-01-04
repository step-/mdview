#!/bin/sh

TEST_LABEL='legacy_index'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text'

. "${0%/*}/lib/common.sh" || exit 1
# compare with --gfm
GFM="$GFM_BIN --smart -e autolink -e strikethrough -e table"
MDVIEW="$MDVIEW_BIN --cm-block-end --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
