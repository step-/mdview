#!/bin/sh

#######################################################################
#                       FOR LEGACY MDVIEW ONLY                        #
#######################################################################

TEST_LABEL='legacy_br'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --no-heading-link"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"
