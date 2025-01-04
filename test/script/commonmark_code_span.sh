#!/bin/sh

TEST_LABEL='commonmark_code_span'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="$subject_file"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"

# CM test range 328-349 covers code span.
# Result summary: all tests PASS conformance except 336 and 337, which
# deviate due to the different way we build paragraph content in
# egg_markdown_add_pending.

# Comment out test id to signal non-conformance to CM specification. Prefix test
# id or test range with 'T' to signal tolerable deviation from cmark output.
test_create () {
	create_cm_conformance "$TEST_LABEL" \
		328-335 \
		T336-337 \
		338-349 \
		;
}

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

main --eol "$@"
