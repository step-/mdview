#!/bin/sh

# make_decl.sh - extract function declarations from a .c file.

# usage:   make_decl.sh [FILE.c...]
# example: make_decl.sh src/*.c src/main.c > all.decl.c

# This program needs fccf - https://github.com/p-ranav/fccf.

### Work around fccf's quirk; source pathnames must include a '/'
n=$#
for p; do
	[ "${p#/}" = "${p}" ] && set -- "$@" "./$p" || set -- "$@" "$p"
done
shift $n

### Extract
fccf --language c --ignore-single-line-results --function '' "$@" |

	### Clean up
	 awk '###awk

# Ignore function body.
/^{/ { body = 1 }
/^}/ { body = 0; next }

# Ignore struct and typedef
# (why does fccf output some structs and typedefs?)
/^[ \t]*(struct|typedef)[ \t]/ { next }

!body {

	# Skip single line comments including fccf`s function line number.
	if ($0 ~ "^//") { next }

	# Skip other single line comments.
	if ($0 ~ "^[ \t]*/[*].*[*]/[ \t]*$") { next }

	# Skip GObject stuff.
	if ($0 ~ /G_DEFINE_/) { next }

	# Erase __attribute__ matches including mispelled ones.
	gsub(/[ \t]*__attribute(__)?\(\([^)]+\)\)/, "")

	# Append ";" to function argument list.
	if($0 ~ /)$/) { sub(/$/, ";") }

	print
}
###awk' |

	### Squeeze runs of whitespace-only lines into a single empty line.
	awk 'NF { print; out = 1 } !NF && out { print ""; out = 0 }' |

	### Sort public before private.
	awk '###awk
BEGIN { RS =  "" }
{ R[NR] = $0 }
END {
	for (i = 1; i <= NR; i++) {
		if (R[i] !~ /^(static|inline static)/) {
			print R[i];
			print ""
		}
	}
	print "/***********************************************************/\n"
	for (i = 1; i <= NR; i++) {
		if (R[i] ~ /^(static|inline static)/) {
			print R[i];
			print ""
		}
	}
}
###awk'

