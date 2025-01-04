#!/bin/sh

TEST_LABEL='auto_lang'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='gui'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="env LANG=en_US.UTF-8 $MDVIEW_BIN --auto-lang"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

