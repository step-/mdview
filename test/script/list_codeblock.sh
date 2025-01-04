#!/bin/sh

TEST_LABEL='list_codeblock'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='ansi html pango text'

. "${0%/*}/lib/common.sh" || exit 1

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

