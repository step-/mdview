This line must include two visible ** asterisks.

<!--------------------------------------------------------------------------------
---- hodgepodge ----------------------------------------------------------------->
*fd64save\**

a <

[*a <*](link)

a >" (we think mdview is correct in not curling a solitary double quote).

`<a name="`">`

foo               bar (http://1.com) <http://2.com>	<u@3.com><http://4.com>baz

`foo``bar``

123 `foo``bar``

[not a `link](/foo`)


`
   a
   b
`

`
c
d`

`e
f
`

`
 x
y
 z
`

[`/root/spot`?](spot.md)

<!------------------------------------------------------------------------------->

<!--------------------------------------------------------------------------------
---- codeblock compiled with CFLAGS+=-UCOMMONMARK_FENCED_CODEBLOCK_LINE_ENDING -->

expect the following HTML markup:
`<pre><code>text in codeblock</code></pre>`
```
text in codeblock
```

<!-- note that an empty line is needed before the indented text in codeblock   -->

again, expect the following HTML markup:
`<pre><code>text in codeblock</code></pre>`

    text in codeblock
<!------------------------------------------------------------------------------->

<!--------------------------------------------------------------------------------

<!-- fenced codeblock with internal tags ---------------------------------------->

```
<a> </a>
```

<!-- autocode followed by autolink ---------------------------------------------->

(http://1.com) <http://2.com>

<!-- email autolink but not autocode -------------------------------------------->

<user@example.com>

<!------------------------------------------------------------------------------->

<!--------------------------------------------------------------------------------
---- protected segments in various contexts ------------------------------------->

# [w](x) ![y](z) `code span`

[w](x) ![y](z) `code span`
-------

* [w](x) ![y](z) `code span`

[w](x) ![y](z) `code span`

    `not code span in code block`

```
`not code span in code block`
```

<!------------------------------------------------------------------------------->

<!--------------------------------------------------------------------------------
---- backslash ------------------------------------------------------------------>

* this line ends with a single backslash \
* this line ends with two backslashes     \\

<!------------------------------------------------------------------------------->
