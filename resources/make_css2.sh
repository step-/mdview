#!/bin/sh

# thanks meson :(
[ "${1##*/}" = 'github-markdown.css' ] &&
	[ "${2##*/}" = 'mtx2.css' ] || {
	echo "usage: $0 .../github-markdown.css .../mtx2.css" >&2
	exit 1
}
set -e
exec 1> "$2"

sed -e 's/markdown-body/mtx-body/' "$1"

printf ".mtx-body .mtx-toc code {\n"
printf "  background-color: transparent;\n"
printf "}\n"
