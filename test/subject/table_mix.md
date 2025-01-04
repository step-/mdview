### DESCRIPTION

**Version note:**  
New mdview versions do not support the code block table feature presented
in the following examples.


<!--
https://www.markdownguide.org/extended-syntax/#tables
-->
mdview supports tables in a limited way: it can recognize table syntax, and it
displays the table as an unstyled code block.  Therefore, if the original table
text is aligned, the output table will look aligned.

Inside the table block all characteristics of a code block apply. For instance,
`_`, `*`, and `+++` represent themselves; quotes don't become smart quotes, and
so on.

### TESTS

-------------------------------------------------------------------------------

**1 TEST aligned table**

| Qty | Food   |
| --- | ------ |
|   1 | mango  |
|  12 | eggs   |

**RESULT** aligned table: "Qty/Food", underlines, "1/mango", "12/eggs"

-------------------------------------------------------------------------------

**2 TEST aligned table, H3 before**

# H3
| Qty | Food   |
| --- | ------ |
|   1 | mango  |
|  12 | eggs   |

**RESULT** heading "H3"; aligned table: "Qty/Food" (thead), "1/mango", "12/eggs"

-------------------------------------------------------------------------------

**3 TEST aligned table (5 rows)**

| Qty | Food   |
| --- | ------ |
|   1 | mango  |
|  12 | eggs   |
row 4
row 5

**RESULT** aligned table: "Qty/Food" (thead), "1/mango", "12/eggs", "row 4/", "row 5/"

-------------------------------------------------------------------------------

**4 TEST table head only**

| Qty | Food   |
| --- | ------ |

**RESULT** aligned table: "Qty/Food" (thead)

-------------------------------------------------------------------------------

**5 TEST no table 1**

| Qty | Food   |
row 2

**RESULT** paragraph "| Qty | Food   | row 2" because underlines are required

-------------------------------------------------------------------------------

**6 TEST table with short underlines (2 rows)**

| Qty | Food   |
| --  | --     |
row 2

**RESULT** aligned table: "Qty/Food" (thead), "row 2/"

-------------------------------------------------------------------------------

**7 TEST alignment markers**

| Qty  | Food   | Life |
| ---: | :----- | :---:|
|   1  | mango  |  5d  |
|  12  | eggs   |  1w  |

**RESULT** right/left/center aligned table: "Qty/Food/Life" (thead), "1/mango/5d", "12/eggs/1w"

-------------------------------------------------------------------------------

**8 TEST no need to align the Markdown table (3 rows)**

| Qty | Food   |
| --- | --- |
|  1 | mango  |
| 12 | eggs   |

**RESULT** aligned rendered table

-------------------------------------------------------------------------------

**9 TEST no left bar (3 rows)**

Qty  | Food   | Life |
---: | :----- | :---:|
  1  | mango  |  5d  |
 12  | eggs   |  1w  |

**RESULT** table

-------------------------------------------------------------------------------

**10 TEST no left bar + indent 2 (3 rows)**

  Qty  | Food   | Life |
  ---: | :----- | :---:|
  1    | mango  |  5d  |
  12   | eggs   |  1w  |

**RESULT** table

-------------------------------------------------------------------------------

**11 TEST no right bar + two trailing spaces (3 rows)**

| Qty  | Food   | Life
| ---: | :----- | :---:
|   1  | mango  |  5d  
|  12  | eggs   |  1w  

**RESULT** table

-------------------------------------------------------------------------------

**12 TEST no side bars ...**

Qty  | Food   | Life
---  | ------ | ----
  1  | mango  |  5d
 12  | eggs   |  1w

**RESULT** table

-------------------------------------------------------------------------------

**13 TEST table preceded by fenced code block (2 rows)**

```
code block
```
| Qty | Food   |
| --- | ------ |
row 2

**RESULT** `code block`; table

-------------------------------------------------------------------------------

**14 TEST table, indented code block before, fenced code block after ...**

    <-- 4 Spaces     code block line 1
	<-- Tab          code block line 2
	| there is an empty markdown line below |    code block line 3
| Qty | Food   |
| --- | ------ |

```
code block
```
line 1

**RESULT** three-line code block, table (thead), `code block`, paragraph "line 1"

_An empty line is needed between the table and the fenced
code block to keep the table from swallowing the code block._

-------------------------------------------------------------------------------

**15 TEST table, indented code block after ...**

| Qty | Food   |
| --- | ------ |
    <-- 4 Spaces     code block line 1
	<-- Tab          code block line 2
line 1

**RESULT** table (thead), two-line code block, paragraph "line 1"

-------------------------------------------------------------------------------

### Test passed if all preceding "RESULT" statements are true
