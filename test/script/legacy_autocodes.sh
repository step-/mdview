#!/bin/sh

TEST_LABEL='legacy_autocodes'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text tty'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-permlink --no-heading-link"
GFM="$GFM_BIN --nobreaks --smart -e strikethrough"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

