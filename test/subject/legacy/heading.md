# Headings

mdview supports two heading styles:

* **ATX-style headings**  
  The entire heading is on a single line.
  The heading starts with a run of one to six leading `#` characters followed by space then the _heading text_ then an optional equal run of `#` characters.
  The number of leading `#` characters corresponds to the heading level.

* **setext-style headings**  
  The _heading text_ can be span multiple lines.
  Level 1 headings end with a line of `=` characters.
  Level 2 headings end with a line of `-` characters.
  The line above the ending line cannot be empty
  Higher-level headings are not possible with this style.

Regardless of the heading style, only the _heading text_ is rendered.

-----------------------------------------

**ATX-STYLE HEADING EXAMPLES**

# Heading 1

Paragraph 1.

## Heading 2

Paragraph 2.

### Heading 3

Paragraph 3.

#### Heading 4

**Bold** paragraph 4.

##### Heading 5

Paragraph 5.

###### Heading 6

Font size of heading levels 5 and 6 is smaller than the default size. Level 4 is at default font size. Levels 1 through 3 are larger than the default font size.

-----------------------------------------

**SETEXT-STYLE HEADING EXAMPLES**

Heading 1
===

Paragraph 1.

Heading 2
---

Paragraph 2.

-----------------------------------------

**SETEXT HEADINGS CAN SPAN MULTIPLE LINES**

A setext-style
level-1
heading
(spread on four lines)
======================

Paragraph 1.

A setext-style  
level-2  
heading  
(spread on four lines with embedded hard line-breaks)
----------------------

Paragraph 2.

-----------------------------------------

**HEADINGS AND WHITE SPACE**

**Markdown** implementations differ in the way white space interacts with headings.
For maximum compatibility, authors should add an empty line before and after the
heading.
