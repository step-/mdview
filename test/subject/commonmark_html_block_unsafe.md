# DESCRIPTION

Commonmark 0.30 20230916  
Test "commonmark_html_block_unsafe" prepared on 2024-01-09.

148 DEVIATING

<!--
<table><tr><td>
<pre>
**Hello**,

_world_.
</pre>
</td></tr></table>
<table><tr><td>
<pre>
**Hello**,
<p><em>world</em>.
</pre></p>
</td></tr></table>
-->

<table><tr><td>
<pre>
**Hello**,

_world_.
</pre>
</td></tr></table>

149 CONFORMING

<!--
<table>
  <tr>
    <td>
           hi
    </td>
  </tr>
</table>

okay.
<table>
  <tr>
    <td>
           hi
    </td>
  </tr>
</table>
<p>okay.</p>
-->

<table>
  <tr>
    <td>
           hi
    </td>
  </tr>
</table>

okay.

150 CONFORMING

<!--
 <div>
  *hello*
         <foo><a>
 <div>
  *hello*
         <foo><a>
-->

 <div>
  *hello*
         <foo><a>

151 CONFORMING

<!--
</div>
*foo*
</div>
*foo*
-->

</div>
*foo*

152 CONFORMING

<!--
<DIV CLASS="foo">

*Markdown*

</DIV>
<DIV CLASS="foo">
<p><em>Markdown</em></p>
</DIV>
-->

<DIV CLASS="foo">

*Markdown*

</DIV>

153 CONFORMING

<!--
<div id="foo"
  class="bar">
</div>
<div id="foo"
  class="bar">
</div>
-->

<div id="foo"
  class="bar">
</div>

154 CONFORMING

<!--
<div id="foo" class="bar
  baz">
</div>
<div id="foo" class="bar
  baz">
</div>
-->

<div id="foo" class="bar
  baz">
</div>

155 CONFORMING

<!--
<div>
*foo*

*bar*
<div>
*foo*
<p><em>bar</em></p>
-->

<div>
*foo*

*bar*

156 CONFORMING

<!--
<div id="foo"
*hi*
<div id="foo"
*hi*
-->

<div id="foo"
*hi*

157 CONFORMING

<!--
<div class
foo
<div class
foo
-->

<div class
foo

158 CONFORMING

<!--
<div *???-&&&-<---
*foo*
<div *???-&&&-<---
*foo*
-->

<div *???-&&&-<---
*foo*

159 CONFORMING

<!--
<div><a href="bar">*foo*</a></div>
<div><a href="bar">*foo*</a></div>
-->

<div><a href="bar">*foo*</a></div>

160 CONFORMING

<!--
<table><tr><td>
foo
</td></tr></table>
<table><tr><td>
foo
</td></tr></table>
-->

<table><tr><td>
foo
</td></tr></table>

161 CONFORMING

<!--
<div></div>
``` c
int x = 33;
```
<div></div>
``` c
int x = 33;
```
-->

<div></div>
``` c
int x = 33;
```

162 CONFORMING

<!--
<a href="foo">
*bar*
</a>
<a href="foo">
*bar*
</a>
-->

<a href="foo">
*bar*
</a>

163 CONFORMING

<!--
<Warning>
*bar*
</Warning>
<Warning>
*bar*
</Warning>
-->

<Warning>
*bar*
</Warning>

164 CONFORMING

<!--
<i class="foo">
*bar*
</i>
<i class="foo">
*bar*
</i>
-->

<i class="foo">
*bar*
</i>

165 CONFORMING

<!--
</ins>
*bar*
</ins>
*bar*
-->

</ins>
*bar*

166 CONFORMING

<!--
<del>
*foo*
</del>
<del>
*foo*
</del>
-->

<del>
*foo*
</del>

167 CONFORMING

<!--
<del>

*foo*

</del>
<del>
<p><em>foo</em></p>
</del>
-->

<del>

*foo*

</del>

168 CONFORMING

<!--
<del>*foo*</del>
<p><del><em>foo</em></del></p>
-->

<del>*foo*</del>

169 CONFORMING

<!--
<pre language="haskell"><code>
import Text.HTML.TagSoup

main :: IO ()
main = print $ parseTags tags
</code></pre>
okay
<pre language="haskell"><code>
import Text.HTML.TagSoup

main :: IO ()
main = print $ parseTags tags
</code></pre>
<p>okay</p>
-->

<pre language="haskell"><code>
import Text.HTML.TagSoup

main :: IO ()
main = print $ parseTags tags
</code></pre>
okay

170 CONFORMING

<!--
<script type="text/javascript">
// JavaScript example

document.getElementById("demo").innerHTML = "Hello JavaScript!";
</script>
okay
<script type="text/javascript">
// JavaScript example

document.getElementById("demo").innerHTML = "Hello JavaScript!";
</script>
<p>okay</p>
-->

<script type="text/javascript">
// JavaScript example

document.getElementById("demo").innerHTML = "Hello JavaScript!";
</script>
okay

171 CONFORMING

<!--
<textarea>

*foo*

_bar_

</textarea>
<textarea>

*foo*

_bar_

</textarea>
-->

<textarea>

*foo*

_bar_

</textarea>

172 CONFORMING

<!--
<style
  type="text/css">
h1 {color:red;}

p {color:blue;}
</style>
okay
<style
  type="text/css">
h1 {color:red;}

p {color:blue;}
</style>
<p>okay</p>
-->

<style
  type="text/css">
h1 {color:red;}

p {color:blue;}
</style>
okay

173 CONFORMING

<!--
<style
  type="text/css">

foo
<style
  type="text/css">

foo
-->

<style
  type="text/css">

foo

174 CONFORMING

<!--
> <div>
> foo

bar
<blockquote>
<div>
foo
</blockquote>
<p>bar</p>
-->

> <div>
> foo

bar

175 CONFORMING

<!--
- <div>
- foo
<ul>
<li>
<div>
</li>
<li>foo</li>
</ul>
-->

- <div>
- foo

176 CONFORMING

<!--
<style>p{color:red;}</style>
*foo*
<style>p{color:red;}</style>
<p><em>foo</em></p>
-->

<style>p{color:red;}</style>
*foo*

177 CONFORMING

<!--
<!-- foo -->*bar*
*baz*
<!-- foo -->*bar*
<p><em>baz</em></p>
-->

<!-- foo -->*bar*
*baz*

178 CONFORMING

<!--
<script>
foo
</script>1. *bar*
<script>
foo
</script>1. *bar*
-->

<script>
foo
</script>1. *bar*

179 CONFORMING

<!--
<!-- Foo

bar
   baz -->
okay
<!-- Foo

bar
   baz -->
<p>okay</p>
-->

<!-- Foo

bar
   baz -->
okay

180 CONFORMING

<!--
<?php

  echo '>';

?>
okay
<?php

  echo '>';

?>
<p>okay</p>
-->

<?php

  echo '>';

?>
okay

181 CONFORMING

<!--
<!DOCTYPE html>
<!DOCTYPE html>
-->

<!DOCTYPE html>

182 CONFORMING

<!--
<![CDATA[
function matchwo(a,b)
{
  if (a < b && a < 0) then {
    return 1;

  } else {

    return 0;
  }
}
]]>
okay
<![CDATA[
function matchwo(a,b)
{
  if (a < b && a < 0) then {
    return 1;

  } else {

    return 0;
  }
}
]]>
<p>okay</p>
-->

<![CDATA[
function matchwo(a,b)
{
  if (a < b && a < 0) then {
    return 1;

  } else {

    return 0;
  }
}
]]>
okay

183 CONFORMING

<!--
  <!-- foo -->

    <!-- foo -->
  <!-- foo -->
<pre><code>&lt;!-- foo --&gt;
</code></pre>
-->

  <!-- foo -->

    <!-- foo -->

184 CONFORMING

<!--
  <div>

    <div>
  <div>
<pre><code>&lt;div&gt;
</code></pre>
-->

  <div>

    <div>

185 CONFORMING

<!--
Foo
<div>
bar
</div>
<p>Foo</p>
<div>
bar
</div>
-->

Foo
<div>
bar
</div>

186 CONFORMING

<!--
<div>
bar
</div>
*foo*
<div>
bar
</div>
*foo*
-->

<div>
bar
</div>
*foo*

187 DEVIATING

<!--
Foo
<a href="bar">
baz
<p>Foo
<a href="bar">
baz</p>
-->

Foo
<a href="bar">
baz

188 CONFORMING

<!--
<div>

*Emphasized* text.

</div>
<div>
<p><em>Emphasized</em> text.</p>
</div>
-->

<div>

*Emphasized* text.

</div>

189 CONFORMING

<!--
<div>
*Emphasized* text.
</div>
<div>
*Emphasized* text.
</div>
-->

<div>
*Emphasized* text.
</div>

190 CONFORMING

<!--
<table>

<tr>

<td>
Hi
</td>

</tr>

</table>
<table>
<tr>
<td>
Hi
</td>
</tr>
</table>
-->

<table>

<tr>

<td>
Hi
</td>

</tr>

</table>

191 CONFORMING

<!--
<table>

  <tr>

    <td>
      Hi
    </td>

  </tr>

</table>
<table>
  <tr>
<pre><code>&lt;td&gt;
  Hi
&lt;/td&gt;
</code></pre>
  </tr>
</table>
-->

<table>

  <tr>

    <td>
      Hi
    </td>

  </tr>

</table>

