#!/bin/sh

TEST_LABEL='special_output_modes_pav2'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='ansi html text tty'

. "${0%/*}/lib/common.sh" || exit 1
# compare with --gfm for strikethrough

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
