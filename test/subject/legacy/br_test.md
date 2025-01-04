### DESCRIPTION

<!--
Legacy:
https://daringfireball.net/projects/markdown/syntax#p
https://stackoverflow.com/a/33191810

New with CommonMark:
https://spec.commonmark.org/0.30/#paragraph
https://spec.commonmark.org/0.30/#blank-lines
-->

Topic: match paragraph lines that end with at least two spaces,
and insert a hard line-break after such lines.

Paragraph lines are text lines that include at least one non-white-space
character, and that aren't headings or ruler or within code blocks.

When a hard line-break is found, the current paragraph remains current, and it
doesn't give rise to a new paragraph as it happens instead when an empty line
is encountered in a paragraph. A hard line-break can also be used with elements
inside the paragraph, and it will not split the element.  For instance, a bullet
list item, in which the hard line-break occurs, does not split the sequential
list in two, and does not split the current paragraph.

**Version note:**  
What happens to a hard-line at the end of a paragraph (or other text block)
depends on the mdview version. New versions that conform to the CommonMark
specification don't render the breaks while legacy mdview does.

### TESTS

<!--
white space in this file is integral to the test
-->

-------------------------------------------------------------------------------

**TEST paragraph 1 ...**

line 1
line 2 
line 3

**RESULT** one text line: "line 1 line 2 line 3"

-------------------------------------------------------------------------------

**TEST paragraph 2 ...**

line 1
line 2  
line 3

**RESULT** two text lines (same paragraph): "line 1 line 2", "line 3"

By contrast, these are two text lines and _two_ paragraphs:

line 1 line 2

line 3

-------------------------------------------------------------------------------

**TEST - with smart quotes ...**

"1"
"2"  
"3"

**RESULT** two text lines (same paragraph): `“1” “2”`, `“3”`

-------------------------------------------------------------------------------

**TEST - bullets ...**

1 br  
2
* 3 (bullet, does not join 2)
* 10
  11 (joins with 10)
* 20  
  21 (below 20)
* 30  
31 (below 30)

**RESULT** HTML:

```html
<p>1<br />
2
</p>
<ul>
<li>3 (bullet, does not join 2)</li>
<li>10 11 (joins with 10)</li>
<li>20<br />
21 (below 20)</li>
<li>30<br />
31 (below 30)</li>
</ul>
```

-------------------------------------------------------------------------------

### Test passed if above all "RESULT" statements are true
