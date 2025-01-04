### CM interaction between blank line and paragraph

**Blank line splits paragraph.**

With blank line between _line 1_ and _line 2_:  
line 1;

line 2.

Without blank line between _line 1_ and _line 2_:  
line 1;
line 2.

### CM interaction between blockcomment and paragraph

**HTML blockcomment splits paragraph.**

With HTML blockcomment between _line 1_ and _line 2_:  
line 1;
<!--
-->
line 2.

Without HTML blockcomment between _line 1_ and _line 2_:  
line 1;
line 2.



------------------------------------------------------------------------



### CM interaction between blank line and code span

**Blank line does not a code span make.**

With blank line between _code_ and _span_:  
``code

span``.

Without blank line between _code_ and _span_:  
``code
span``.

### CM interaction between blockcomment and code span

**HTML blockcomment does not a code span make.**

With HTML blockcomment between _code_ and _span_:  
``code
<!--
-->
span``

Without HTML blockcomment between _code_ and _span_:  
``code
span``.



------------------------------------------------------------------------



### CM interaction between blank line and bullet list

**tight list and loose list:** mdview doesn't distinguish between the two kinds.

* tight 1
* tight 2

**loose list**

A blank line between _loose 2_ and _loose a_ doesn't split the list,
nor add a line ending or start a new block.

* loose 1
* loose 2 blank line next [BR]  

* loose a
* loose b

loose list ended.

### CM interaction between blockcomment and bullet list

* loose 1,1
* loose 1,2 blockcomment next
<!--
-->
* loose 2,1
* loose 2,2



------------------------------------------------------------------------



### A bee in my bonnet...

* tight 1
* tight 2
### H3
