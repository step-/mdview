#!/bin/sh

TEST_LABEL='commonmark_127'
subject_file="test/subject/$TEST_LABEL.md"
TEST_ARGS="$subject_file"
TEST_FORMATS='html'

. "${0%/*}/lib/common.sh" || exit 1
MDVIEW="$MDVIEW_BIN --cm-block-end --no-smart --no-ext --no-heading-link"

# CM test range 119-147 covers fenced codeblock.
# Test 127 must run alone to test support for run-away codeblock.

# Result summary: PASS.

# Omit test id to signal non-conformance to CM specification. Prefix test id or
# test range with 'T' to signal tolerable deviation from cmark output.
test_create () { create_cm_conformance "$TEST_LABEL" 127; }

test_diff () { diff_cm_conformance "$1" "$2" "$subject_file"; }

main --eol "$@"
