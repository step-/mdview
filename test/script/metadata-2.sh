#!/bin/sh

TEST_LABEL='metadata-2'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='text'

. "${0%/*}/lib/common.sh" || exit 1

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

