#!/bin/sh

set -e

SPEC="${0%/*}/spec_test_cases.json"
[ -s "$SPEC" ]
# spec version, download time and download URL
read SPEC_VER SPEC_TIME SPEC_URL < "${0%/*}/spec_test_cases_version"
: "${SPEC_VER:?}" "${SPEC_TIME:?}" "${SPEC_URL:?}"
MDROOT="test/subject"
HTROOT="test/reference"

usage () {
cat << EOF
Usage: [env ENVIRONMENT] ${0##*/} TEST_LABEL ( TEST-RANGE | TEST# )+

TEST#      ::= <tolerate><digit>+
TEST-RANGE ::= <tolerate><digit>+'-'<digit>+
<tolerate> ::= 'T' | ''

Prefix TEST# or TEST-RANGE with 'T' to label the test / all the tests in the
range as tolerable deviations. Regardless the 'T' prefix the test script will
report FAIL should any single test case deviate from the specification.

Take MDVIEW_BIN and REDIR2 from ENVIRONMENT - see common.sh.

Example: ${0##*/} "commonmark_code_span" 328-349
EOF
}

make_test_list () { # $@-( TEST-RANGE | TEST# )+
	awk -v IN="$*" '
###awk
BEGIN {
	na = split(IN, a)
	for (i = 1; i <= na; i++) {
		nr = split(a[i], r, "-")
		tpre = sub(/^T/, "",  r[1])
		max = r[2] ? r[2] : r[1]
		for (j = r[1]; j <= max; j++)
			printf " %s%d", tpre ?"T" :"", j
	}
	printf "\n"
}
###awk'
}

prepare_commonmark_test_range () { # $1-TEST_LABEL $2@-tests
# $2@ is list of TEST# that identify JSON objects having key "example:" in $SPEC
	local TEST_LABEL="${1:?}"; shift
	local k v md ht
	local mdf htf


	. "${0%/*}/common.sh" || return 1

	mdf="$MDROOT/${TEST_LABEL//[[:space:]]/_}.md"
	htf="$HTROOT/${TEST_LABEL//[[:space:]]/_}.html"

	# don't clobber!
	# [ -e "$mdf" ] && echo >&2 "refusing to clobber $mdf !" && exit 1 | :
	# [ -e "$htf" ] && echo >&2 "refusing to clobber $htf !" && exit 1 | :

	printf '# DESCRIPTION\n\nCommonmark %s %s  \nTest "%s" prepared on %s.\n\n' \
		"$SPEC_VER" "$SPEC_TIME" "$TEST_LABEL" "${TEST_PREPARED_ON:-$(date +%Y-%m-%d)}" > "$mdf"

	if [ -z "$REDIR2" ]; then
		"$MDVIEW_BIN" --html "$mdf" > "$htf"
	else
		"$MDVIEW_BIN" --html "$mdf" > "$htf" 2> "$REDIR2"
	fi

	exec 3>> "$mdf" 4>> "$htf"

	while read -r k v; do
		unset n T
		case $k in
			'"markdown":' ) # test-subject
				md="${v#?}"; md="${md%?,}"; md="${md//%/%%}"
				;;
			'"html":'     ) # reference-output
				ht="${v#?}"; ht="${ht%?,}"; ht="${ht//%/%%}"
				;;
			'"example":'  ) # test-number
				n="${v%,}"
				;;
		esac

		### output the test case as two files, markdown (.md) and HTML markup (.html):
		# -----------------------------------------
		# .md file includes for each test-number:
		# -----------------------------------------
		#    test-number grading-annotation
		#    <!--
		#    test-subject (markdown)
		#    reference-output (html)
		#    -->
		#    (intentional empty line, work around https://github.com/mity/md4c/issues/200)
		#    test-subject
		# -----------------------------------------
		# .html file includes for each test-number:
		# -----------------------------------------
		#    <p>test-number grading-annotation</p><!--
		#    test-subject (markdown)
		#    -->
		#    reference-output (html)
		# -----------------------------------------
		# GRADING (grading-annotation):
		# A test author enters grading-annotation as either:
		# "CONFORMING" (default), when mdview's actual html output
		# for test-number is identical to to cmark's - the reference
		# implementation of the CommonMark specification; or
		# "DEVIATING", which means that mdview can't achive conformance
		# on test-number but the deviation from the specification is
		# acceptable (this is a subjective point).
		# To enter the DEVIATING grading-annotation as a script option,
		# prefix the test-number or test-number range with "T", e.g.,
		#     $0 1 T2 3-5 T7-8 9 ... # deviating: 2, 7, 8

		case " $* " in
			(*" $n "*) T='CONFORMING';;
			(*) case " $* " in
				(*" T$n "*) T='DEVIATING';;
				(*) continue;; # test number is outside the test set $*
			esac;;
		esac
		printf '%d %s\n\n<!--\n'"$md$ht"'-->\n\n'"$md"'\n' $n "$T" >&3 # .md
		printf '<p>%d %s</p><!--\n'"$md"'-->\n'"$ht" $n "$T" >&4 # .html
	done < "$SPEC"

	exec 3>&- 4>&-
}

if [ $# -eq 0 -o "$1" = -h -o "$1" = "--help" ]; then
	usage
	exit 0
fi

label="${1:?}"
shift
: "${*:?}"
set -- $(make_test_list $*)
prepare_commonmark_test_range "$label" $*

