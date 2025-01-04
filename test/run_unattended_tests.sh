#!/bin/sh

# usage $0 [TEST-SCRIPT...]

TEST_LABEL=dummy
. "${0%/*}/script/lib/common.sh" || exit 1

export REDIR2=${REDIR2:-/dev/null}

mdview_project_id=$(get_mdview_project_id)

[ $# -gt 0 ] || set -- test/script/*.sh
for p in "$@"; do
	# skip test cases specific to legacy mdview features
	if [ $mdview_project_id -eq 2 ]; then
	case "${p##*/}" in
		(gui_*)
		printf "\033[7mpango test '%s' skipped (view it in mdview)\033[0m\n" "${p##*/}" >&2
		continue
		;;
	esac
	fi
	$p
done |
"${0%/*}/highlight_diff.awk"

