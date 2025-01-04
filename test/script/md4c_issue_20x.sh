#!/bin/sh

TEST_LABEL='md4c_issue_20x'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

# ----------------------------------------------------------------------------
# Test requires --unsafe-html to be DISABLED (not included in mdview options).
# ----------------------------------------------------------------------------

# This test is expected to FAIL as long as
# https://github.com/mity/md4c/issues/200 will remain open.

main "$@"
