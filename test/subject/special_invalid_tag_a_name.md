
<!--
white space in this file matters for the test
-->
### DESCRIPTION

This test is about excluding HTML "anchors" from output, that is, all and only
markdown lines that consist of validly-formed HTML tag `<a name="some name">`.

_Note that this file includes invalid HTML tags._

### TESTS

-------------------------------------------------------------------------------

**TEST INVALID TAGS...**

<a name=></a>
<a name= ></a>

**RESULT** The "TEST ..." heading followed by a line with two invalid `<a>` tags.

-------------------------------------------------------------------------------

**TEST `<a>` tag without `name` attribute...**

<a></a>
<a ></a>
<a href=""></a>

**RESULT** Just a line consisting of the "TEST ..." heading.

-------------------------------------------------------------------------------

**TEST `<a>` tags that are followed by non-white space...**

<a name></a>suffix
<a name > </a>suffix
<a name> </a> suffix

**RESULT** A line consisting of the "TEST ..." heading
followed by a line consisting of "suffix" repeated three times.

-------------------------------------------------------------------------------

**TEST valid anchors: show nothing until the next RESULT**

<a name></a>
<a name=""></a>
<a name="n"></a>
<a name="sp" > </a>
  	 <a name=""></a>
  	 <a name="n"></a>
  	 <a name="sp" > </a>
<a name=""></a>  	 
<a name="n"></a>  	 
<a name="sp"></a>  	 
  	 <a name=""></a>  	 
  	 <a name="n"></a>  	 
  	 <a name="sp" > </a>  	 

**RESULT** A line consisting of the "TEST ..." heading

-------------------------------------------------------------------------------

**TEST anchors in code blocks: show them as code**

        <a name="show me"></a>

```
<a name="show me"></a>
```

**RESULT** two lines each consisting of `<a name="show me"></a>`.

-------------------------------------------------------------------------------

### Test passed if all preceding "RESULT" statements are true
