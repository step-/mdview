# DESCRIPTION

Commonmark 0.30 20230916  
Test "code span" prepared on 2023-11-17.

328 CONFORMING

<!--
<p><code>foo</code></p>
-->
`foo`

329 CONFORMING

<!--
<p><code>foo ` bar</code></p>
-->
`` foo ` bar ``

330 CONFORMING

<!--
<p><code>``</code></p>
-->
` `` `

331 CONFORMING

<!--
<p><code> `` </code></p>
-->
`  ``  `

332 CONFORMING

<!--
<p><code> a</code></p>
-->
` a`

333 CONFORMING

<!--
<p><code> b </code></p>
-->
` b `

334 CONFORMING

<!--
<p><code> </code>
<code>  </code></p>
-->
` `
`  `

335 CONFORMING

<!--
<p><code>foo bar   baz</code></p>
-->
``
foo
bar  
baz
``

336 DEVIATING

<!--
<p><code>foo </code></p>
-->
``
foo 
``

337 DEVIATING

<!--
<p><code>foo   bar  baz</code></p>
-->
`foo   bar 
baz`

338 CONFORMING

<!--
<p><code>foo\</code>bar`</p>
-->
`foo\`bar`

339 CONFORMING

<!--
<p><code>foo`bar</code></p>
-->
``foo`bar``

340 CONFORMING

<!--
<p><code>foo `` bar</code></p>
-->
` foo `` bar `

341 CONFORMING

<!--
<p>*foo<code>*</code></p>
-->
*foo`*`

342 CONFORMING

<!--
<p>[not a <code>link](/foo</code>)</p>
-->
[not a `link](/foo`)

343 CONFORMING

<!--
<p><code>&lt;a href=&quot;</code>&quot;&gt;`</p>
-->
`<a href="`">`

344 CONFORMING

<!--
<p><a href="`">`</p>
-->
<a href="`">`

345 CONFORMING

<!--
<p><code>&lt;http://foo.bar.</code>baz&gt;`</p>
-->
`<http://foo.bar.`baz>`

346 CONFORMING

<!--
<p><a href="http://foo.bar.%60baz">http://foo.bar.`baz</a>`</p>
-->
<http://foo.bar.`baz>`

347 CONFORMING

<!--
<p>```foo``</p>
-->
```foo``

348 CONFORMING

<!--
<p>`foo</p>
-->
`foo

349 CONFORMING

<!--
<p>`foo<code>bar</code></p>
-->
`foo``bar``

