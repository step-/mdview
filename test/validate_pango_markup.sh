#!/bin/sh

png_dir=
case "$1" in
	-h|--help)
		echo "usage: $0 [--png=/path/to/output/dir] test/subject/file.md"
		exit 0
	;;
	--png)
		png_dir=1
		shift 1
		;;
	--png=?*)
		png_dir="${1#--png=}"
		mkdir -p "$png_dir" || exit 1
		shift 1
		;;
esac

TEST_LABEL="${0##*/}"
. "${0%/*}/script/lib/common.sh" || exit 1

tmpd="${TMPDIR:-/tmp}/${0##*/}"
mkdir -p "$tmpd" || exit 1

REDIR2=${REDIR2:-/dev/null}

"$MDVIEW_BIN" --help | grep -qFm1 -- '--pango'
if [ $? -ne 0 ]; then
	printf "\033[7m  %s  \033[0m %s\n" \
	"INVALID" "$TEST_LABEL" \
	"$MDVIEW_BIN --pango: required option not found"
	printf "\033[7m  %s  \033[0m %s\n" \
	"INFO" "$TEST_LABEL" \
	"Compile $MDVIEW_BIN with -DOPT_PANGO (see Makefile's \"test\" target)"
	exit 1
fi
if ! command -v "$PANGO_VIEW_BIN" > /dev/null; then
	printf "\033[7m  %s  \033[0m %s %s\n" \
	"INVALID" "$TEST_LABEL" \
	"$PANGO_VIEW_BIN (pango-view) not installed"
	printf "\033[7m  %s  \033[0m %s\n" \
	"INFO" "$TEST_LABEL" \
	"Install the pango-view command"
	exit 1
fi

STATUS=0

[ $# -gt 0 ] || set -- test/subject/*.md
for p in "$@"; do
	tmpf="$tmpd/${p##*/}.pango"
	if   [ -d "$png_dir" ]; then pngf="$png_dir/${tmpf##*/}.png"
	elif [ -n "$png_dir" ]; then pngf="$tmpd/${tmpf##*/}.png"
	else pngf=
	fi
	"$MDVIEW_BIN" --pango $p > "$tmpf" 2> "$REDIR2" &&
		"$PANGO_VIEW_BIN" -q --markup --serialize-to="$tmpf.json" \
		"$tmpf" 2> "$tmpf.err"

	failed=$?
	if [ $failed -eq 0 ]; then
		printf "\033[7m  %s  \033[0m %s %s\n" \
		"PASS" "$TEST_LABEL" "$p"

		if [ -n "$pngf" ]; then
			"$PANGO_VIEW_BIN" --dpi 72 --markup -q \
				--serialized "$tmpf.json" -o "$pngf"
		fi

		rm "$tmpf" "$tmpf.json" "$tmpf.err"
	else
		STATUS=1
		printf "\033[7;31m  %s  \033[0m %s %s\n%s\n" \
		"FAIL $failed" "$TEST_LABEL" "$p" "$(cat "$tmpf.err")"
	fi
done

if [ $STATUS -ne 0 ]; then
	printf "\033[7;31m  %s  \033[0m %s %s\n" \
	"INFO" "$TEST_LABEL" "error logs in '$tmpd'"
fi
exit $STATUS
