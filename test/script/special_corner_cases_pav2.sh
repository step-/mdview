#!/bin/sh

TEST_LABEL='special_corner_cases_pav2'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-ext --cm-block-end"
CMARK="$CMARK_BIN --smart --nobreaks"
GFM="$GFM_BIN --smart --nobreaks"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
