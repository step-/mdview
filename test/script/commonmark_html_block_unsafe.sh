#!/bin/sh

TEST_LABEL='commonmark_html_block_unsafe'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="$subject_file"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --unsafe-html --no-heading-link"
CMARK="$CMARK_BIN --nobreaks --unsafe"
GFM="$GFM_BIN --nobreaks --unsafe"

export NO_HTML_TIDY_CHECK=1

# CM test range 148-191 covers html blocks.
# Result summary: all tests PASS. The tolerable divergencies below are due to
# newline differences.

# Comment out test id to signal non-conformance to CM specification. Prefix test
# id or test range with 'T' to signal tolerable deviation from cmark output.
test_create () {
	create_cm_conformance "$TEST_LABEL" \
		T148 \
		149-186 \
		T187 \
		188-191 \
		;
}

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

main --eol "$@"
