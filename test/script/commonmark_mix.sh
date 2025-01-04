#!/bin/sh

TEST_LABEL='commonmark_mix'
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html text tty'

. "${0%/*}/lib/common.sh" || exit 1
mdview_project_id=$(get_mdview_project_id)
[ "$mdview_project_id" -lt 2 ] && legacy_opt='--no-gettext' || unset legacy_opt
MDVIEW="$MDVIEW_BIN --soft-break $legacy_opt --cm-block-end --no-smart --no-heading-link"
CMARK="$CMARK_BIN"

# test_create () { create_default "$@"; }
# test_diff () { diff_default "$1" "$2"; }

main "$@"

