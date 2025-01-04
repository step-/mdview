#!/bin/sh

TEST_LABEL='commonmark_emphasis'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="$subject_file"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-ext --no-heading-link"
CMARK="$CMARK_BIN --nobreaks --smart"

# CM test range 350-480 cover emphasis and strong emphasis.
# Result summary:
# All test cases PASS conformance with the MTX parser.
# With the legacy parser 6 of 131 tests FAIL conformance, and three of them
# even blank the pango viewer.  The reference file was grandfathered into 100%
# pass (by running "$0 --create") in order to record the current state of
# matters. However, this script forces option --cmark for the legacy parser to
# make clear the failing test cases. (When option --cmark is used, the reference
# file is ignored.)

# Comment out test id to signal non-conformance to CM specification. Prefix test
# id or test range with 'T' to signal tolerable deviation from cmark output.
test_create () { create_cm_conformance "$TEST_LABEL" 350-480; }

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

mdview_project_id=$(get_mdview_project_id)
if [ $mdview_project_id -gt 1 ]; then
	main --eol "$@"
else
	# Ignore reference file and make clear which test cases blank mdview.
	main --eol "$@" --cmark

: << \EOF
#######################################################################
#               THREE TEST CASES BLANK THE PANGO VIEWER               #
#######################################################################

RUN> ./mdview --cm-block-end --no-ext test/subject/commonmark_emphasis.md  --cmark
SELECTION OF POSSIBLY SERIOUS ERRORS:
case  429: <p><em><strong>foo</em> bar</strong></p>␤
        ***foo* bar**
        <p><strong><em>foo</em> bar</strong></p>
case  430: <p><strong>foo <em>bar</strong></em></p>␤
        **foo *bar***
        <p><strong>foo <em>bar</em></strong></p>
case  469: <p>*foo <strong>bar <em>baz bim</strong> bam</em></p>␤
        *foo __bar *baz bim__ bam*
        <p><em>foo <strong>bar *baz bim</strong> bam</em></p>
total test cases                         :  131
line diff count - reference vs. result   :    6
non-conforming cases                     :    6   4.58 %
known deviations                         :    0   0.00 %
  FAIL 1   commonmark_emphasis html
EOF

fi
