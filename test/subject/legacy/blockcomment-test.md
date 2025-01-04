# TESTS

-------------------------------------------------------------------------------

BEGINS NEXT LINE
begin

<!-- this one-line comment should not be shown -->
<!-- neither this one --> <!-- nor this one -->
<!--
This multi-line comment should not be shown.
Next line doesn't start a fenced codeblock
```
This is comment text to be hidden, not code to be shown.
```
-->
<!-- Inline comment followed by multi-line comment --> <!-- IGNORED UP TO END OF THIS LINE...
This and successive lines form a regular text paragraph because they are outside
the previous HTML block comment, which started and ended on the same line.  
The end-of-comment sequence `-` `-` `>` also lies outside the block, therefore it's
regular text:
including the end-of-block sequence
-->

```
First line inside codeblock.
<!-- This line isn't a comment and will be shown. -->
<!--
So will this line.
-->
Last line inside codeblock.
```

end
ENDS PRIOR LINE

-------------------------------------------------------------------------------

regular text
<!--
Test: paragraph + block comment + H3
There once was a bug that squashed everything into a single H3 heading, e.g.,
        <h3>regular text h3 text<h3>
but not anymore.
-->
### h3 text

-------------------------------------------------------------------------------

A block comment
inside a paragraph
<!--
-->
splits the
paragraph in two.

-------------------------------------------------------------------------------

* A block comment
* inside a bullet list
<!--
-->
* splits the
* list in two.

-------------------------------------------------------------------------------
