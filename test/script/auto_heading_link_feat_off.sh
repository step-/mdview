#!/bin/sh

TEST_LABEL='auto_heading_link_feat_off'
TEST_ARGS="test/subject/${TEST_LABEL%_off}.md"
TEST_FORMATS='pango'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

