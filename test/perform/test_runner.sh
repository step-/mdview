#!/bin/sh

BUILD=${BUILD:-1}
OPTS=${OPTS-"--pango,--no-extensions --exit-test,--exit-test"}
OPTS=${OPTS-"--pango,--exit-test"}
MDVIEW=${MDVIEW:-"./mdview"}
HYPERFINE=${HYPERFINE:-"hyperfine --warmup 1"}
RUNS=${RUNS:-1}
GRAPH=${GRAPH:-0}

### Optional, uncomment to use ###
# gprof needs binary build with '-pg -g'
# gprof2dot generates SVG call graph
# extrance was only used while developing this script
unset extrace gprof gprof2dot
gprof=gprof
gprof2dot=gprof2dot
#extrace="extrace -t -f -o $tracelog"

case "$1" in (-h|--help) cat << USAGE
USAGE:    [ENVVARS] test_runner.sh [FILE.md ...] | tee -a results.md
- If FILE.md is omitted, a default file list is used, see "case" in main().
ENVVARS
- BUILD=0
	disable running 'make' before the test runs
- MDVIEW="$MDVIEW"
	you shouldn't need to pass options but you may
- HYPERFINE="$HYPERFINE"
- OPTS="$OPTS"
	comma-separated list of mdview options for hyperfine
- RUNS=$RUNS
	hyperfine -M
- GRAPH=$GRAPH
        run grof and gprof2dot repeating the test run
USAGE
	exit
esac

ts=$(date +%Y%m%d_%H%M%S)
APPDIR="$(cd "${0%/*}"; command pwd -P)"
APPNAME="$(basename "$0" .sh)"
tmps="${TMPDIR:?}/$APPNAME"
buildlog="${tmps}_build.log" tracelog="${tmps}_trace.log"

build () {
	[ $BUILD -eq 0 ] && return 0
	local ret
	printf '%s\n' "### $ts commit $(git describe --tags)" "\`$*\`" '' &&
	(
	make clean && `#compiledb.sh` \
		make "$@"
	) > "$buildlog" 2>&1
	ret=$?
	$MDVIEW --help | tail -n 1
	echo
	cat "$buildlog" >&2
	return $ret
}

run_test_hyperfine () { # $@-test_file(s) ; IN: $OPTS
	local p=${1:?}
	# build hyperfine parameter lists for mdview arguments
	local mdf; for p; do mdf="$mdf,$p"; done; mdf="${mdf#,}"

	$extrace $HYPERFINE -M$RUNS --export-csv -\
		-L opts "$OPTS" -L mdf "$mdf" "$MDVIEW {opts} {mdf}"
}

convert_csv_to_md_table () {
	awk -F, -v "MDVIEW=$MDVIEW" -v "RUNS=$RUNS" '
###awk

#1      ,2   ,3     ,4     ,5   ,6     ,-3 ,-2 ,-1           ,-0
#command,mean,stddev,median,user,system,min,max,parameter_mdf,parameter_opts
BEGIN  {
	min = 1e12
	r = c = status = 0
}
/Warning:|Error:/ {
	status = 1
}
NF > 2 {
	# blank some columns (they will be dropped upon saving to T)
	$4 = $(NF-0) = $(NF-1) = $(NF-2) = $(NF-3) = ""

#A	# reuse column for command exit status
#A	$(NF-1) = r == 0 ? "exit" : status

	# squeeze 'command' text
	$1 = substr($1, index($1,"--"))
	sub(/ .*\//, " ", $1)
	if (r == 0) {
		$1 = sprintf("%s  [runs: %d, time: ms]", MDVIEW, RUNS)
	}

	# update min for relative time calculation in END
	if ($2 < min) {
		min = $2
	}

	# save data cells to T (column names for r == 0)
	++r
	c = 0
	for (i = 1; i <= NF; i++) {
		if ($i != "") {
			T[r, ++c] = $i
		}
	}

	if (status != 0) {
		exit status
	}
}
END {
	# markdown table heading
	for (j = 1; j <= c; j++) {
		printf "| %s ", T[1, j]
	}
	print "| rel |"
	printf "|:---" # 'command'
	for (j = 2; j <= c ; j++) {
#A	for (j = 2; j < c ; j++) {
		printf "|---:"
	}
	print "|---:|" # 'rel'

	# markdown table body
	for (i = 2; i <= r; i++) {
		printf "| %s ", T[i, 1]
		for (j = 2; j <= c; j++) {
			printf "| %.1f ", T[i, j] * 1000
		}
#A		# fill status column
#A		printf "| %d ", T[i, c ]
		# append relative time column
		printf "| %.2f |\n", T[i, 2] / min
	}
}
###awk'
}

run_test () { # $@-test-file(s)
	local ret
	run_test_hyperfine "$@"
	ret=$?
	echo
	[ -n "$extrace" ] && grep 'mdview' "$tracelog" >&2
	return $ret
}

gprof_test () {
	[ -z "$gprof" ] && return 0
	local mdf=${1:?}
	local bn="$(basename "$mdf" .${mdf##*.})"
	$gprof -b ${MDVIEW%% *} gmon.out > "$APPDIR/prof/$bn-$ts.txt" &&
		cp gmon.out "$APPDIR/prof/$bn-$ts.gmon.out"
}

graph_test () {
	[ -z "$gprof2dot" ] && return 0
	# pip3 install gprof2dot
	local mdf=${1:?}
	local bn="$(basename "$mdf" .${mdf##*.})"
	$gprof ${MDVIEW%% *} "$APPDIR/prof/$bn-$ts.gmon.out" |
		$gprof2dot |
		dot -Tsvg -o "$APPDIR/prof/$bn-$ts.svg"
		# SVG output is searchable
}

main () { # [$@-markdown_file ...]
	local datasrc=default
	[ $# -gt 0 ] && datasrc=args
	case $datasrc in
		args)   : ;;
		default) set -- test/perform/subject/*.md ;;
		custom) set -- \
			test/perform/subject/cv-influences.md \
			;;
		*)      return 0 ;;
	esac

	rm -f "${tmps:?}"*

	build "CFLAGS=-UMTX_DEBUG -UMTX_TEXT_VIEW_DEBUG -UVIEWER_DEBUG \
		-DOPT_PANGO -DOPT_EXIT_TEST -pg -ggdb3 -O3" &&

	# run all tests and output a summary table
	run_test "$@" 2>&1 | tee /dev/tty | convert_csv_to_md_table &&

	# repeat each test individually collecting profiling data and graphs
	if [ $GRAPH -ne 0 ] && [ -n "$gprof" ] && [ -n "$gprof2dot" ]; then
		for p; do
			echo "::::: $p :::::" >&2
			run_test "$p" &&
				gprof_test "$p" &&
				graph_test "$p" &&
			: || exit 1
		done
	fi &&
	ret=$?

	ls -l "$tmps"* >&2
	return $ret
}

main "$@"
