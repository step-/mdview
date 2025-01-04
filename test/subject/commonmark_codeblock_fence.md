# DESCRIPTION

Commonmark 0.30 20230916  
Test "codeblock fence" prepared on 2023-11-17.

119 CONFORMING

<!--
<pre><code>&lt;
 &gt;
</code></pre>
-->

```
<
 >
```

120 CONFORMING

<!--
<pre><code>&lt;
 &gt;
</code></pre>
-->

~~~
<
 >
~~~

121 CONFORMING

<!--
<p><code>foo</code></p>
-->

``
foo
``

122 CONFORMING

<!--
<pre><code>aaa
~~~
</code></pre>
-->

```
aaa
~~~
```

123 CONFORMING

<!--
<pre><code>aaa
```
</code></pre>
-->

~~~
aaa
```
~~~

124 CONFORMING

<!--
<pre><code>aaa
```
</code></pre>
-->

````
aaa
```
``````

125 CONFORMING

<!--
<pre><code>aaa
~~~
</code></pre>
-->

~~~~
aaa
~~~
~~~~

129 CONFORMING

<!--
<pre><code>
  
</code></pre>
-->

```

  
```

130 CONFORMING

<!--
<pre><code></code></pre>
-->

```
```

131 CONFORMING

<!--
<pre><code>aaa
aaa
</code></pre>
-->

 ```
 aaa
aaa
```

132 CONFORMING

<!--
<pre><code>aaa
aaa
aaa
</code></pre>
-->

  ```
aaa
  aaa
aaa
  ```

133 CONFORMING

<!--
<pre><code>aaa
 aaa
aaa
</code></pre>
-->

   ```
   aaa
    aaa
  aaa
   ```

134 CONFORMING

<!--
<pre><code>```
aaa
```
</code></pre>
-->

    ```
    aaa
    ```

135 CONFORMING

<!--
<pre><code>aaa
</code></pre>
-->

```
aaa
  ```

136 CONFORMING

<!--
<pre><code>aaa
</code></pre>
-->

   ```
aaa
  ```

138 CONFORMING

<!--
<p><code> </code>
aaa</p>
-->

``` ```
aaa

140 CONFORMING

<!--
<p>foo</p>
<pre><code>bar
</code></pre>
<p>baz</p>
-->

foo
```
bar
```
baz

141 CONFORMING

<!--
<h2>foo</h2>
<pre><code>bar
</code></pre>
<h1>baz</h1>
-->

foo
---
~~~
bar
~~~
# baz

142 CONFORMING

<!--
<pre><code class="language-ruby">def foo(x)
  return 3
end
</code></pre>
-->

```ruby
def foo(x)
  return 3
end
```

143 CONFORMING

<!--
<pre><code class="language-ruby">def foo(x)
  return 3
end
</code></pre>
-->

~~~~    ruby startline=3 $%@#$
def foo(x)
  return 3
end
~~~~~~~

144 CONFORMING

<!--
<pre><code class="language-;"></code></pre>
-->

````;
````

145 CONFORMING

<!--
<p><code>aa</code>
foo</p>
-->

``` aa ```
foo

146 CONFORMING

<!--
<pre><code class="language-aa">foo
</code></pre>
-->

~~~ aa ``` ~~~
foo
~~~

147 CONFORMING

<!--
<pre><code>``` aaa
</code></pre>
-->

```
``` aaa
```

