#!/bin/sh
# each test script sources this file to import main() and its dependencies.

MDVIEW_BIN="${MDVIEW_BIN:-./mdview}"
CMARK_BIN="${CMARK_BIN:-cmark}"
GFM_BIN="${GFM_BIN:-cmark-gfm}"
PANDOC_BIN="${PANDOC_BIN:-pandoc}"
PANGO_VIEW_BIN="${PANGO_VIEW_BIN:-pango-view}"

TESTSCRIPTNAME="${0##*/}"

__usage () { # {{{1
	cat << EOF
Usage: [env ENVIRONMENT] \\
       ${0##*/} [OPTIONS] [FORMAT...] [ | highlight_diff.awk ]

Optionally, pipe output to 'highlight_diff.awk' to colorize differences.

ENVIRONMENT DEFAULTS:
$(column -t -s '@' << ENV
- REDIR2=@redirect \$MDVIEW_BIN 2> \$REDIR2 if set
- MDVIEW_BIN=$MDVIEW_BIN
- CMARK_BIN=$CMARK_BIN@optional for comparisons
- GFM_BIN=$GFM_BIN@optional for comparisons
- PANDOC_BIN=$PANDOC_BIN@optional for comparisons
- PANGO_VIEW_BIN=$PANGO_VIEW_BIN@optional for validate-pango-markup.sh
ENV
)

$(fmt -80 << MAN
For each FORMAT: $TESTSCRIPTNAME (this script) runs mdview on a SUBJECT file
to produce a RESULT file for the output FORMAT; then it compares an existing
REFERENCE file against the RESULT file and reports differences. The test run is
marked PASS if no differences can be found, otherwise it is marked FAIL. This
script exists non-zero if at least one run was marked FAIL or if other errors
occurred.
Typically:
- Write SUBJECT file (markdown).
- Pass --create to create a REFERENCE file for each specified FORMAT.
- Run $TESTSCRIPTNAME to test regressions or non-conformance.
Optionally:
- Edit $TESTSCRIPTNAME to change test FORMAT(s).
- Edit $TESTSCRIPTNAME to change the default comparison procedure.[1]
- Edit $TESTSCRIPTNAME to change the default creation procedure.[2]

[1] Change the body of function 'test_diff'. Choose a diff_* function from file
'common.sh' or write your own function instead, and call it in 'test_diff'.

[2] Change the body of function 'test_create'. Choose a predefined create_*
function from file 'common.sh' or write your own function instead, and call it
in 'test_create'.
MAN
)

FORMATS:
  ansi, html (default), pango (DEBUG build only), text, tty.
  gui (similar to option --gui)

OPTIONS:
  Unrecognized options are passed to function 'test_diff'.
  --cmark   Compare cmark[1] against mdview - strikethrough not supported.
  --create  Create REFERENCE file - overwriting without prompt.
  --eol     Insert '␤' before each compared line ending to reveal end-of-line.
  --gfm     Compare cmark-gfm[2] against mdview - strikethrough is supported.
  --gui     Start mdview GUI passing the test options; no test results.
  --pandoc  Compare pandoc[3] against mdview.
  --vim     Inspect differences with vim instead of test_diff(), and PASS test.

[1] https://github.com/commonmark/cmark
[2] https://github.com/github/cmark-gfm
[3] https://github.com/jgm/pandoc
EOF
}

# Set traps and create temporary folder. {{{1
TMPD=$(mktemp -d -p "${TMPDIR:-/tmp}" "${0##*/}_XXXXXX")
# created with permissions u+rwx minus umask restrictions
chmod 700 "$TMPD" || exit 1
trap 'command rm -rf "'"${TMPD:?}"'"; exit $STATUS' \
	HUP INT ALRM TERM 0 # USR1 USR2
mkdir -p "$TMPD"

# {{{1}}}

# DO NOT CHANGE COMMAND VALUES HERE!
# if you need to change do it in the test script, not here, and
# change AFTER sourcing this file.
# To find common ground between legacy mdview, MTX mdview, cmark and cmark-gfm
# invoke cmark{-gfm} with --nobreaks and mdview without --soft-break.
# - the legacy parser joins paragraph lines with ' ' and it can't do otherwise
# - the MTX parser does the same but can also join lines with '\n' if
#   invoked with --soft-break
# - cmark{,-gfm} can join lines with ' ' if invoked with --nobreaks
# - the legacy parser ignores unknown options, such as --soft-break.
# Leaving legacy mdview out of the picture, common ground is also reached
# by invoking MTX mdview with --soft-break and cmark{,-gfm} without
# --nobreaks.
# For cmark-gfm and MTX mdview, table support is even when cmark-gfm is
# invoked with `-e table`.
MDVIEW="${MDVIEW:-$MDVIEW_BIN}"
CMARK="${CMARK:-$CMARK_BIN --smart --nobreaks}"
GFM="${GFM:-$GFM_BIN --smart --nobreaks -e strikethrough}"
PANDOC="${PANDOC:-$PANDOC_BIN -f markdown-smart-auto_identifiers -t html}"

# The test script initializes TEST_LABEL.
REFERENCE_DIR='test/reference'
REFERENCE_STEM="${REFERENCE_DIR:?}/${TEST_LABEL:?}"
RESULT_STEM="$TMPD/${TEST_LABEL:?}"

show_line_ending () {
local script
# U+2424 symbol for newline
[ -n "$_opt_eol" ] && script='s/$/␤/' || script=''
sed -e "$script" "$@"
}

# $@-main_args
test_create () {
	create_default "$@"
}

### create reference file from subject file
# $@-main_args
create_default () {
	write_reference_files "$@"
}

### create subject and reference file from commonmark spec
# $1-short_description $2@-commonmark_example_id... (list of)
# commonmark_example_id from CM specification, single number <n> or range <n>-<m>
# optionally prepend 'T', e.g. T<n> or T<n>-<m> to mark acceptable deviations
create_cm_conformance () {
	# create subject file
	test/script/lib/prepare_commonmark_test_range.sh "$@" &&
		# create reference file
		write_reference_files html
}

# The test script initializes TEST_ARGS, TEST_LABEL
# $@-main-args
write_reference_files () {
	: "ensure: ${TEST_LABEL:?} ${MDVIEW:?} ${REFERENCE_STEM:?}"
	local p ret=0
	for p in "$@"; do
		# sift valid test formats
		case $p in
			ansi|html|pango|text|tty) :;;
			*) echo "error: invalid reference file format: $p" >&2
			########
			false
			########
		esac &&
			: echo "RUN $MDVIEW $TEST_ARGS --$p" >&2 &&
			if [ -z "$REDIR2" ]; then
				$MDVIEW $TEST_ARGS --$p > "$REFERENCE_STEM.$p"
			else
				$MDVIEW $TEST_ARGS --$p > "$REFERENCE_STEM.$p" 2> "$REDIR2"
			fi # NO &&
		if [ $? -eq 0 ]; then
			printf "\033[7m  %s  \033[0m %s %s\n" "CREATED" "$TEST_LABEL" "$p"
		else
			printf "\033[7;31m  %s  \033[0m %s %s\n" "ERROR" "$TEST_LABEL" "$p"
			ret=1
		fi
	done
return $ret
}

# $* ansi|html|text|tty ...
# The test script initializes TEST_ARGS, TEST_FORMATS
run_test () { # => REFERENCE_STEM
	: "?? activate the helper that simply starts the viewer ??"
		if [ "${TEST_FORMATS%*gui}" != "$TEST_FORMATS" ] || [ -n "$_opt_viewer" ]; then
			echo "RUN> $MDVIEW $TEST_ARGS $mdview_opts" >&2
			$MDVIEW $TEST_ARGS $mdview_opts
			exit $?
		fi

	: "no, then ensure: ${MDVIEW:?} ${RESULT_STEM:?}"
	local p ret=0 mdview_opts
	for p in ${@:-$TEST_FORMATS}; do
		mdview_opts=
		[ -n "$_opt_cmark" ] && mdview_opts="$mdview_opts --cm-block-end"

		########################
		#  output result file  #
		########################
		if [ -z "$REDIR2" ]; then
			printf "%s" "RUN> $MDVIEW $TEST_ARGS $mdview_opts --$p" >&2
			printf " %s\n" "> $RESULT_STEM.$p" >&2
			$MDVIEW $TEST_ARGS $mdview_opts --$p \
			> "$RESULT_STEM.$p"
		else
			printf "%s" "RUN> $MDVIEW $TEST_ARGS $mdview_opts --$p" >&2
			printf " %s\n" "> $RESULT_STEM.$p 2> \"$REDIR2\"" >&2
			$MDVIEW $TEST_ARGS $mdview_opts --$p \
			> "$RESULT_STEM.$p" 2> "$REDIR2"
		fi
		[ $? -eq 0 ] || ret=1


		##################################################
		#  output alternative reference file (optional)  #
		##################################################
		if [ -n "$_opt_cmark" ]; then
			: "ensure: ${CMARK:?}"
			if [ $p = html ]; then
				echo "RUN< $CMARK ${TEST_ARGS##* }" >&2
				$CMARK ${TEST_ARGS##* } |
					massage_cmark_output "$RESULT_STEM.cmark.$p"
				[ $? -eq 0 ] || ret=1
				REFERENCE_STEM="$RESULT_STEM.cmark"
			fi


		elif [ -n "$_opt_gfm" ]; then
			: "ensure ${GFM:?}"
			if [ $p = html ]; then
				echo "RUN< $GFM ${TEST_ARGS##* }" >&2
				$GFM ${TEST_ARGS##* } |
					massage_cmark_output "$RESULT_STEM.gfm.$p"
				[ $? -eq 0 ] || ret=1
				REFERENCE_STEM="$RESULT_STEM.gfm"
			fi


		elif [ -n "$_opt_pandoc" ]; then
			: "ensure: ${PANDOC:?}"
			if [ $p = html ]; then
				:; echo "RUN< $PANDOC ${TEST_ARGS##* }" >&2
				$PANDOC ${TEST_ARGS##* } > "$RESULT_STEM.pandoc.$p"
				[ $? -eq 0 ] || ret=1
				REFERENCE_STEM="$RESULT_STEM.pandoc"
			elif [ $p = text ]; then
				$PANDOC_BIN -f markdown -t plain ${TEST_ARGS##* } > "$RESULT_STEM.pandoc.$p"
				[ $? -eq 0 ] || ret=1
				REFERENCE_STEM="$RESULT_STEM.pandoc"
			fi
		fi
	done
	return $ret
}

massage_cmark_output () {
	# slurp the stream
	gawk -v 'RS=a\\yb' -v OUT="$1" '###gawk{{{
	{
		# cmark --smart replaces ellipsis but mdview does not
		gsub(/…/, "...")
		# they go n-dash where we go m-dash
		gsub(/–/, "—")
		# they may end the stream with an extra \n, we do not
		sub(/\n\n$/, "")

		# (obsoleted by mdview option --soft-break)
		# This is the most important difference: cmark joins
		# lines inside <p> or <li> with line feeds but mdview
		# uses spaces. We try to make this unnoticeable by
		# replacing line feed with space as needed.
		buf = $0
		sub(/\n$/, "", buf)
		gsub (/<br \/>\n/, "HARDBREAK_LINEFEED", buf)
		alen = split(buf, a, /<p>|<\/p>/, sep)
		buf  = ""
		for (i = 1; i <= alen; i++) {
			if (sep[i - 1] == "<p>")
				gsub ("\n", " ", a[i])
			buf = (buf sep[i - 1] a[i])
		}
		gsub ("HARDBREAK_LINEFEED", "<br />\n", buf)
		print buf > OUT
		close(OUT)
	}
	END { exit !(NR > 0) }
	###gawk}}}'
}
# $* html|tty|text ...
run_test_diffs () {
	: "foreach {x} in ($*) ensure ${REFERENCE_STEM:?}.{x} ${RESULT_STEM:?}.{x}"
	local ret differ reff tmpf
	[ -n "$_opt_vim" ] && differ=vimdiff || differ=test_diff
	for p in "${@:-html}"; do
			reff="$REFERENCE_STEM.$p" tmpf="$RESULT_STEM.$p" &&
			if ! [ -e "$reff" ]; then
				>&2 printf "error: no reference file: pass --create to initialize: %s\n" "$reff"
				ret=1
			else
				show_line_ending "$reff" > "$tmpf.e.nl" &&
				show_line_ending "$tmpf" > "$tmpf.a.nl" &&
				: echo "CALL $differ $_opts_test_diff \"$tmpf.e.nl\" \"$tmpf.a.nl\"" >&2 &&
				$differ $_opts_test_diff "$tmpf.e.nl" "$tmpf.a.nl"
			[ $? -eq 0 ] || ret=1
			fi
	done
	return $ret
}

# $1-reference_file $2-result_file
test_diff () {
	diff_default "${1:?}" "${2:?}"
}

# $1-reference_file $2-result_file
diff_default () {
	diff_git_word_diff "${1:?}" "${2:?}"
}

# $1-reference_file $2-result_file
diff_plain () {
	diff "${1:?}" "${2:?}"
}

# $1-reference_file $2-result_file
diff_y () {
	diff -y --suppress-common-lines "${1:?}" "${2:?}"
}

# $1-reference_file $2-result_file
diff_git_word_diff () {
	git -P diff --word-diff "$1" "$2"
}

# $1-reference_file $2-result_file $3-subject_file
# for test files prepared by test/prepare_commonmark_test_range.sh
# use test script option --eof for accurate results
diff_cm_conformance () {
	if [ -n "$_opt_vim" ]; then
		vimdiff "${1:?}" "${2:?}"
	else
		sub_diff_cm_conformance "${1:?}" "${2:?}" "${3:?}"
	fi

	return $?
}

# $1-reference_file $2-result_file $3-subject_file
# !!! For accurate results run the test script with option --eol !!!
sub_diff_cm_conformance () {
	# $2 is located in $TMPDIR
	local referencef="${1:?}" resultf="${2:?}" subjectf="${3:?}"

	local capture cases test_case_header_regex
	test_case_header_regex='(<p>)?([0-9]+) (DEVIATING|CONFORMING)'
	cases=$(grep -Ec "$test_case_header_regex" "$referencef")

	# capture stderr leaving stdout alone
	{ capture=$(\
	git -P diff --no-index --word-diff=porcelain -- "$referencef" "$resultf" |
		gawk \
		-v TEST_CASE_HEADER_REGEX="$test_case_header_regex" \
		-v CASES=$cases '
###gawk {{{
##### purpose: minimize git diff output for humans

BEGIN {
	cases=CASES+0
	non_conforming_cases = deviating_cases = start = 0
}

# git diff headers
/^--- a\// || /^\+\+\+ b\// { next }

# differences before first test case header
! start && /^[-+]/ {
	print
	++non_conforming_cases
}

# end previous block
$0 ~ TEST_CASE_HEADER_REGEX && start > 0 {
	if (show) {
		show = 0
		do_show()
	}
}
function do_show(   a) {
	# do not count a small deviation from CM spec as FAIL
	match (R[start], TEST_CASE_HEADER_REGEX, a)
	if (a[3] == "DEVIATING") {
		--non_conforming_cases
		++deviating_cases
	}

	for (i = start; i < NR; i++)
		print R[i]
}

# start new block
$0 ~ TEST_CASE_HEADER_REGEX {
	delete R; start = NR; show = 0
}

start > 0 {
	R[NR] = $0
	# remember to show the current block iff it differs
	if (!show && $0 ~ /^[-+]/)
		show = ++non_conforming_cases
}

END {
	if (show)
		do_show()

	printf "%d %d %.2f %d %.2f\n", cases, \
		non_conforming_cases, non_conforming_cases * 100 / cases, \
		deviating_cases, deviating_cases * 100 / cases \
	> "/dev/stderr"
	exit non_conforming_cases > 0
}
###gawk }}}' 2>&1 >&3 3>&-); } 3>&1

	### "validate" output with html-tidy: ###
	# -mute less-serious warning types; unmuted types could be serious errors,
	# such as improperly nested tags, i.e. <em><strong></em></strong>
	local tidy_status=0
	if [ -z "$NO_HTML_TIDY_CHECK" ]; then
		tidy -eqi -file "$resultf.err" \
		--doctype omit --vertical-space no --show-body-only yes --wrap 0 --mute-id yes \
		--mute MISSING_ATTRIBUTE,MISSING_TITLE_ELEMENT,\
NESTED_EMPHASIS,COERCE_TO_ENDTAG,DISCARDING_UNEXPECTED \
		"$resultf"
		tidy_status=$?
	fi

	# report tidy warnings/errors in context
	if [ -s "$resultf.err" ]; then
		>&2 gawk \
			-v TEST_CASE_HEADER_REGEX="$test_case_header_regex" '
###gawk {{{
BEGINFILE { ++fnum }
fnum == 1 { S[FNR] = $0; next } # subject file   (markdown)
fnum == 2 { R[FNR] = $0; next } # reference file (cmark)
fnum == 3 { I[FNR] = $0; next } # result file    (mdview)
fnum == 4 {                     # error file     (tidy)
# error line example: line 1 column 1 - Warning: inserting implicit <body> (INSERTING_TAG)
	if ($0 ~ /<body>/) next
	lineno = $2+0
	E[lineno] = (E[lineno] "\n" $0)
}
END {
	# read test case number a[2] from block comment
	max = length(S)
	for (i = 1; i <= max; i++) {
		if (match(S[i], TEST_CASE_HEADER_REGEX, a)) {
			testnum = a[2]
			if (1 == index(S[i+2], "<!--")) {
				for (j = i + 3; j <= max; j++) {
					if (1 == index(S[j], "-->")) {
						i = j
						break
					}
					T[testnum] = T[testnum] "\n\t" S[j]
				}
			}
		}
	}
}
END {
	if (length(E) > 0)
		print "SELECTION OF POSSIBLY SERIOUS ERRORS:"
	for (lineno in E) {
		#print (substr(E[lineno], 2) "\n\t" I[lineno])
		#printf "line % 4s: %s\n", lineno, I[lineno]
		testnum = lineno_to_testnum(lineno)
		printf "case % 4s: %s%s\n", testnum, I[lineno], T[testnum]
	}
}
function lineno_to_testnum (lineno,   a) {
	while (--lineno > 0)
		if (match(R[lineno], TEST_CASE_HEADER_REGEX, a))
			return a[2]
	return -1
}
###gawk }}}' \
			"$subjectf" "$referencef" "$resultf" "$resultf.err"

	fi

	### summarize results ###

	# using diff -U to count diffs is accurate as long as the two files are quite similar
	local dff=$(diff -U 0 "$1" "$2" | grep ^@ -c)

	set -- $capture
	local tc=$1 ncc=$2 nccpc=$3 dvc=$4 dvcpc=$5

	set --
	set -- "$@" 'total test cases' 0 "$tc" ""
	set -- "$@" 'line diff count - reference vs. result'
		[ $dff -eq 0 ] && set -- "$@" 0 || set -- "$@" 31; set -- "$@" "$dff" ""
	set -- "$@" 'non-conforming cases'
		[ $ncc -eq 0 ] && set -- "$@" 0 || set -- "$@" 31; set -- "$@" "$ncc" "$nccpc %"
	set -- "$@" 'known deviations' 0 $dvc "$dvcpc %"

	printf >&2 '%-40s : \033[%dm% 4d\033[0m %8s\n' "$@"

	[ $ncc -eq 0 -a $tidy_status -ne 2 ]
}

# Return a conventional ID to distinguish mdview3 from mdview4
get_mdview_project_id () { # <= $MDVIEW_BIN => 1(mdview3) 2(mdview4)
	local ifs
	: "${MDVIEW_BIN:?}"
	[ -n "$MDVIEW_BIN" ] || return 1
	set -- $("$MDVIEW_BIN" --version)
	ifs="$IFS"; IFS=.
	set -- $2
	IFS="$ifs"
	[ "$1" -ge 2024 ] && set 2 || set 1
	echo $1
}

# optional: $* ansi|html|tty|text (valid test formats)
# $* overrides TEST_FORMATS.
# The test script initializes TEST_ARGS, TEST_LABEL, TEST_FORMATS.
main () { # => $STATUS
	: "ensure: ${TEST_LABEL:?}"
	local p failed
	unset _opts_test_diff \
	_opt_cmark \
	_opt_create \
	_opt_eol \
	_opt_gfm \
	_opt_pandoc \
	_opt_viewer \
	_opt_vim \
	;
	[ -n "$TEST_FORMATS" ] || TEST_FORMATS='html'

	while ! [ "${1#-}" = "$1" ]; do
		case "$1" in
			-h|--help) __usage; STATUS=0 exit ;;
			--cmark  ) _opt_cmark=1
				;;
			--create ) OVERWRITE_TEST_FILES=1 ;;
			--eol    ) _opt_eol=1 ;;
			--gfm    ) _opt_gfm=1 ;;
			--pandoc ) _opt_pandoc=1
			;;
			--gui    ) _opt_viewer=1 ;;
			--vim    ) _opt_vim=1 ;;
			-*       )
				echo >&2 "OPTION> $1 will be passed to function test_diff"
				_opts_test_diff="$_opts_test_diff $1"
				;;
		esac
		shift
	done

	STATUS=0
	export G_DEBUG=fatal-warnings

	# create reference file iff option --create
	if [ -n "$OVERWRITE_TEST_FILES" ]; then
		if ! test_create ${@:-$TEST_FORMATS}; then
			STATUS=1
			exit $STATUS
		fi
	fi

	for p in ${@:-$TEST_FORMATS}; do
		case $p in
			# To skip unsupported formats by PROGRAM append $_opt_PROGRAM.
			ansi|pango) [ -z "$_opt_cmark$_opt_gfm$_opt_pandoc" ] || continue ;;
			html) : ;; # common ground among mdview, cmark, cmark-gfm and pandoc
			text) [ -z "$_opt_cmark$_opt_gfm" ] || continue ;;
			tty ) [ -z "$_opt_cmark$_opt_gfm$_opt_pandoc" ] || continue ;;
			gui ) : "pass-through to GUI viewer" ;;
			*   ) : "unknown test format, let run_test report error"
			# run_test will report "error: no reference file"
			# run_test_diffs will report "error: invalid reference link format: $p"
			# NOTE: for a new test format to be valid it must be
			# registered here and in run_test_diffs.
		esac

		run_test "$p"
		if [ $? -ne 0 ]; then
			printf "\033[7m  %s  \033[0m %s %s\n" \
			"INVALID" "$TEST_LABEL" "$p"
			false
		else
			run_test_diffs "$p"
			failed=$?
			if [ $failed -eq 0 ]; then
				printf "\033[7m  %s  \033[0m %s %s\n" \
				"PASS" "$TEST_LABEL" "$p"
			else
				STATUS=1
				printf "\033[7;31m  %s  \033[0m %s %s\n" \
				"FAIL $failed" "$TEST_LABEL" "$p"
			fi
		fi
	done
	return $STATUS
}

