#!/bin/sh

TEST_LABEL='commonmark_codeblock_fence'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="test/subject/$TEST_LABEL.md"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"
CMARK="$CMARK_BIN --nobreaks"

# CM test range 119-147 covers fenced codeblock.
# Result summary: All test cases PASS.
# Tests 126, 127, 128, 137, 139 live in their own test script.

# Comment out test id to signal non-conformance to CM specification. Prefix test
# id or test range with 'T' to signal tolerable deviation from cmark output.
test_create () {
	create_cm_conformance "$TEST_LABEL" \
		119-125 \
		129-136 \
		138 \
		140-147 \
		;
}

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

main --eol "$@"
