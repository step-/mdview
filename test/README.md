# MDVIEW MTX Tests

## Requirements

1. `make mdview TEST=1` (see [INSTALL])
2. Install the [pango-view] command.

## Quickstart

`make test` runs from the top directory:

    test/run_unattended_tests.sh            [test/script/{s1,s2,...}.sh  ]
    test/validate_pango_markup.sh [--png]   [test/subject/{s1,s2,...}.md ]

With option `--png`, `validate_pango_markup.sh` saves a PNG image of the
rendered markdown document to directory `$TMPDIR/validate_pango_markup.sh`
(or a different location with `--png=/other/directory`).

## Getting specific

All test scripts support the same set of command-line options:

    test/script/auto_heading_link.sh --help

Run a single test:

    test/script/TestScriptName.sh

View rendered Markdown (no run test):

    test/script/TestScriptName.sh --gui

Run multiple tests:

    test/run_unattended_tests.sh test/script/TestScriptName1.sh ...

## Manual tests

```sh
    mdview test/gui_relative_path/index.md &

    for p in test/subject/gui_*.md; do mdview $p & done
```

## Directory structure

From the top directory:

```
test/doc/        Licenses and the precious CommonMark specification.

test/perform/    [Performance tests].

test/script/     Shell scripts - each file runs a single test.
                 All files share a common structure and use the test engine.

test/script/lib/ The test engine.

test/subject/    Markdown files - each file corresponds to a script file.
                 You write a test subject file.
                 Treat a test subject as the test source file.

test/reference/  HTML files - each file corresponds to a subject file.
                 You create the reference **on demand** (`--create`).
                 Create a reference when the corresponding subject changes.
                 After `--create`, the test script will always report `PASS`.
```

## Performance tests

**Requirements**

1. Install [hyperfine] for test profiling.
2. Optionally install `gprof`¹ and [gprof2dot] to graph profiling data.

**Running test**

Note that some individual tests can run for a couple of minutes.

To run all the tests and save a summary Markdown table:

```sh
test/perform/test_runner.sh > test/perform/stats/$(date +%Y%m%d).md
```

The `test_runner.sh` script allows for saving and graphing profiling data
using `gprof` and `gprof2dot`. Refer to `test/perform/test_runner.sh --help`
for instructions.

Folder structure from the top directory:

```
test/perform/prof/             Collected profiling data.
test/perform/stats/            Collected statistics.
test/perform/subject/          Markdown files.
test/perform/test_runner.sh    Run test(s) collecting profiling data and stats.
```

## Miscellany

The `test/alias.sh` file includes the aliases I use to compare mdview with
other programs (cmark, etc.).

## Footnotes

¹ Preinstalled in Fatdog64 if the devx SFS loaded.


[pango-view]: <https://packages.debian.org/stable/pango1.0-tools>
[INSTALL]:    <../INSTALL.md>
[hyperfine]:  <https://github.com/sharkdp/hyperfine>
[gprof2dot]:  <https://pypi.org/project/gprof2dot/>

