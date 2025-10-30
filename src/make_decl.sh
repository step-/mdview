#!/bin/sh

case "$1" in -h|--help)
cat << EOF
${0##*/} - extract function declarations from .c files.
This program needs fccf - https://github.com/p-ranav/fccf.

Usage: [env FCCF_OPTIONS=...] $0 [FILE.c...]

Example: $0 src/*.c > all.decl.c

If the output syntax is invalid, likely it is due to ${0##*/} reading
a right curly bracket '}' in column 1 within a function body. Indenting
the bracket in the source code will fix the output syntax. Column-1
brackets are reserved for definitions and declarations.
EOF
	exit
esac

### Work around fccf's quirk; source pathnames must include a '/'
n=$#
for p; do
	[ "${p#/}" = "${p}" ] && set -- "$@" "./$p" || set -- "$@" "$p"
done
shift $n

### Extract
fccf --language c --ignore-single-line-results --function '' \
	$FCCF_OPTIONS "$@" |

	### Clean up
	 awk '###awk

# Ignore function body.
/^{/ { body = 1 }
/^}/ { body = 0; next }

# Possibly was the struct/typedef brace in column 1 ???? XXX
# # Ignore struct and typedef
# # (why does fccf output some structs and typedefs?)
# /^[ \t]*(struct|typedef)[ \t]/ { next }

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

