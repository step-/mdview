#!/bin/sh

TEST_LABEL='commonmark_html_block'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="$subject_file"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"
CMARK="$CMARK_BIN --nobreaks"
GFM="$GFM_BIN --nobreaks"

# CM test range 148-191 covers html blocks.
# Result summary: all tests PASS.

# Comment out test id to signal non-conformance to CM specification. Prefix test
# id or test range with 'T' to signal tolerable deviation from cmark output.
test_create () {
	create_cm_conformance "$TEST_LABEL" \
		148-191 \
		;
}

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

main --eol "$@"
