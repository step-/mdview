#!/bin/sh

TEST_LABEL='gui_link_delete_em_strong'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='gui'

. "${0%/*}/lib/common.sh" || exit 1

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

