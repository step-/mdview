#!/bin/sh

set -e

install_target="${MESON_INSTALL_DESTDIR_PREFIX:?}/share/man/man1/mdview.1"
mdview_bin="${MESON_BUILD_ROOT:?}/src/mdview"

cd "${MESON_SOURCE_ROOT:?}/doc"

config="${MESON_BUILD_ROOT:?}/src/mtxversion.h"
if [ -z "$PACKAGE_DESC" ]; then
	# Extract package description from the configuration file.
	PACKAGE_DESC="$(awk '
###awk
match($0, /^#define PACKAGE_DESC[ \t]+/) {
	print substr($0, RSTART + RLENGTH)
	exit
}
###awk' "$config")"
	PACKAGE_DESC="${PACKAGE_DESC#\"}"
	PACKAGE_DESC="${PACKAGE_DESC%\"}"
fi
export PACKAGE_DESC
mkdir -p "$(dirname "$install_target")"
./make_mdview_1.sh ./mdview.1.h2m "$install_target" "$mdview_bin"

