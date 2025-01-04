#!/bin/sh

TEST_LABEL='commonmark_smart_text'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-ext --no-heading-link"
CMARK="$CMARK_BIN --nobreaks --smart"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

