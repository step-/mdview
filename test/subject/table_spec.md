Column 1 | Column 2
---------|---------
*foo*    | bar
**baz**  | [qux]
quux     | [quuz](/url2)

[qux]: /url

Basic table example of a table with two columns and three lines (when not
counting the header) is as follows:

| Column 1 | Column 2 |
|----------|----------|
| foo      | bar      |
| baz      | qux      |
| quux     | quuz     |

The leading and succeeding pipe characters (`|`) on each line are optional:

Column 1 | Column 2 |
---------|--------- |
foo      | bar      |
baz      | qux      |
quux     | quuz     |

| Column 1 | Column 2
|----------|---------
| foo      | bar
| baz      | qux
| quux     | quuz

Column 1 | Column 2
---------|---------
foo      | bar
baz      | qux
quux     | quuz

However for one-column table, at least one pipe has to be used in the table
header underline, otherwise it would be parsed as a Setext title followed by
a paragraph.

Column 1 (SETEXT)
--------
foo
baz
quux

Leading and trailing whitespace in a table cell is ignored and the columns do
not need to be aligned.

Column 1 |Column 2
---|---
foo | bar
baz| qux
quux|quuz

The table cannot interrupt a paragraph.

**FAIL wrt cmark-gfm**  
Lorem ipsum dolor sit amet.
| Column 1 | Column 2
| ---------|---------
| foo      | bar
| baz      | qux
| quux     | quuz

Similarly, paragraph cannot interrupt a table:

Column 1 | Column 2
---------|---------
foo      | bar
baz      | qux
quux     | quuz
Lorem ipsum dolor sit amet.

The first, the last or both the first and the last dash in each column
underline can be replaced with a colon (`:`) to request left, right or middle
alignment of the respective column:

| Column 1 | Column 2 | Column 3 | Column 4 |
|----------|:---------|:--------:|---------:|
| default  | left     | center   | right    |

To include a literal pipe character in any cell, it has to be escaped.

Column 1 | Column 2
---------|---------
foo      | bar
baz      | qux \| xyzzy
quux     | quuz

Contents of each cell is parsed as an inline text which may contents any
inline Markdown spans like emphasis, strong emphasis, links etc.

Column 1 | Column 2
---------|---------
*foo*    | bar
**baz**  | [qux]
quux     | [quuz](/url2)

[qux]: /url

**FAIL wrt cmark-gfm**  
However pipes which are inside a code span are not recognized as cell
boundaries.

Column 1 | Column 2
---------|---------
`foo     | bar`
baz      | qux
quux     | quuz
