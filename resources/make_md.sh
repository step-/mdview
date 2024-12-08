#!/bin/sh

# thanks meson :(
[ "${1%.md.in}" != "$1" ] && [ "${2%.md}" != "$2" ] || {
	echo "usage: $0 .../FILE.md.in .../FILE.md [MESON_BUILD_ROOT]" >&2
	exit 1
}
set -e
exec 1> "$2"

#
# Note that setting SUB_CPPFLAGS only matters for a Debug build.
# Use the Makefile-based build for Debug builds.
#

[ -d "$3" ] && versionh="$3/src/mtxversion.h" ||   # meson
	versionh=../src/mtxversion.h               # Makefile

cpp $SUB_CPPFLAGS -include $versionh \
	-DHBRK="\\" -traditional -P "$1" | awk 'NF {p=1} p'
