#!/bin/sh

TEST_LABEL='table_link_url'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html ansi text tty pango'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --unsafe-html"
GFM="$GFM_BIN --nobreaks --smart -e table"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

