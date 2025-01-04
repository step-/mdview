#!/bin/sh

TEST_LABEL='auto_heading_link_bogus'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='pango'

. "${0%/*}/lib/common.sh" || exit 1

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

