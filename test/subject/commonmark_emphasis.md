# DESCRIPTION

Commonmark 0.30 20230916  
Test "commonmark_emphasis" prepared on 2024-01-28.

350 CONFORMING

<!--
*foo bar*
<p><em>foo bar</em></p>
-->

*foo bar*

351 CONFORMING

<!--
a * foo bar*
<p>a * foo bar*</p>
-->

a * foo bar*

352 CONFORMING

<!--
a*"foo"*
<p>a*&quot;foo&quot;*</p>
-->

a*"foo"*

353 CONFORMING

<!--
* a *
<p>* a *</p>
-->

* a *

354 CONFORMING

<!--
foo*bar*
<p>foo<em>bar</em></p>
-->

foo*bar*

355 CONFORMING

<!--
5*6*78
<p>5<em>6</em>78</p>
-->

5*6*78

356 CONFORMING

<!--
_foo bar_
<p><em>foo bar</em></p>
-->

_foo bar_

357 CONFORMING

<!--
_ foo bar_
<p>_ foo bar_</p>
-->

_ foo bar_

358 CONFORMING

<!--
a_"foo"_
<p>a_&quot;foo&quot;_</p>
-->

a_"foo"_

359 CONFORMING

<!--
foo_bar_
<p>foo_bar_</p>
-->

foo_bar_

360 CONFORMING

<!--
5_6_78
<p>5_6_78</p>
-->

5_6_78

361 CONFORMING

<!--
пристаням_стремятся_
<p>пристаням_стремятся_</p>
-->

пристаням_стремятся_

362 CONFORMING

<!--
aa_"bb"_cc
<p>aa_&quot;bb&quot;_cc</p>
-->

aa_"bb"_cc

363 CONFORMING

<!--
foo-_(bar)_
<p>foo-<em>(bar)</em></p>
-->

foo-_(bar)_

364 CONFORMING

<!--
_foo*
<p>_foo*</p>
-->

_foo*

365 CONFORMING

<!--
*foo bar *
<p>*foo bar *</p>
-->

*foo bar *

366 CONFORMING

<!--
*foo bar
*
<p>*foo bar
*</p>
-->

*foo bar
*

367 CONFORMING

<!--
*(*foo)
<p>*(*foo)</p>
-->

*(*foo)

368 CONFORMING

<!--
*(*foo*)*
<p><em>(<em>foo</em>)</em></p>
-->

*(*foo*)*

369 CONFORMING

<!--
*foo*bar
<p><em>foo</em>bar</p>
-->

*foo*bar

370 CONFORMING

<!--
_foo bar _
<p>_foo bar _</p>
-->

_foo bar _

371 CONFORMING

<!--
_(_foo)
<p>_(_foo)</p>
-->

_(_foo)

372 CONFORMING

<!--
_(_foo_)_
<p><em>(<em>foo</em>)</em></p>
-->

_(_foo_)_

373 CONFORMING

<!--
_foo_bar
<p>_foo_bar</p>
-->

_foo_bar

374 CONFORMING

<!--
_пристаням_стремятся
<p>_пристаням_стремятся</p>
-->

_пристаням_стремятся

375 CONFORMING

<!--
_foo_bar_baz_
<p><em>foo_bar_baz</em></p>
-->

_foo_bar_baz_

376 CONFORMING

<!--
_(bar)_.
<p><em>(bar)</em>.</p>
-->

_(bar)_.

377 CONFORMING

<!--
**foo bar**
<p><strong>foo bar</strong></p>
-->

**foo bar**

378 CONFORMING

<!--
** foo bar**
<p>** foo bar**</p>
-->

** foo bar**

379 CONFORMING

<!--
a**"foo"**
<p>a**&quot;foo&quot;**</p>
-->

a**"foo"**

380 CONFORMING

<!--
foo**bar**
<p>foo<strong>bar</strong></p>
-->

foo**bar**

381 CONFORMING

<!--
__foo bar__
<p><strong>foo bar</strong></p>
-->

__foo bar__

382 CONFORMING

<!--
__ foo bar__
<p>__ foo bar__</p>
-->

__ foo bar__

383 CONFORMING

<!--
__
foo bar__
<p>__
foo bar__</p>
-->

__
foo bar__

384 CONFORMING

<!--
a__"foo"__
<p>a__&quot;foo&quot;__</p>
-->

a__"foo"__

385 CONFORMING

<!--
foo__bar__
<p>foo__bar__</p>
-->

foo__bar__

386 CONFORMING

<!--
5__6__78
<p>5__6__78</p>
-->

5__6__78

387 CONFORMING

<!--
пристаням__стремятся__
<p>пристаням__стремятся__</p>
-->

пристаням__стремятся__

388 CONFORMING

<!--
__foo, __bar__, baz__
<p><strong>foo, <strong>bar</strong>, baz</strong></p>
-->

__foo, __bar__, baz__

389 CONFORMING

<!--
foo-__(bar)__
<p>foo-<strong>(bar)</strong></p>
-->

foo-__(bar)__

390 CONFORMING

<!--
**foo bar **
<p>**foo bar **</p>
-->

**foo bar **

391 CONFORMING

<!--
**(**foo)
<p>**(**foo)</p>
-->

**(**foo)

392 CONFORMING

<!--
*(**foo**)*
<p><em>(<strong>foo</strong>)</em></p>
-->

*(**foo**)*

393 CONFORMING

<!--
**Gomphocarpus (*Gomphocarpus physocarpus*, syn.
*Asclepias physocarpa*)**
<p><strong>Gomphocarpus (<em>Gomphocarpus physocarpus</em>, syn.
<em>Asclepias physocarpa</em>)</strong></p>
-->

**Gomphocarpus (*Gomphocarpus physocarpus*, syn.
*Asclepias physocarpa*)**

394 CONFORMING

<!--
**foo "*bar*" foo**
<p><strong>foo &quot;<em>bar</em>&quot; foo</strong></p>
-->

**foo "*bar*" foo**

395 CONFORMING

<!--
**foo**bar
<p><strong>foo</strong>bar</p>
-->

**foo**bar

396 CONFORMING

<!--
__foo bar __
<p>__foo bar __</p>
-->

__foo bar __

397 CONFORMING

<!--
__(__foo)
<p>__(__foo)</p>
-->

__(__foo)

398 CONFORMING

<!--
_(__foo__)_
<p><em>(<strong>foo</strong>)</em></p>
-->

_(__foo__)_

399 CONFORMING

<!--
__foo__bar
<p>__foo__bar</p>
-->

__foo__bar

400 CONFORMING

<!--
__пристаням__стремятся
<p>__пристаням__стремятся</p>
-->

__пристаням__стремятся

401 CONFORMING

<!--
__foo__bar__baz__
<p><strong>foo__bar__baz</strong></p>
-->

__foo__bar__baz__

402 CONFORMING

<!--
__(bar)__.
<p><strong>(bar)</strong>.</p>
-->

__(bar)__.

403 CONFORMING

<!--
*foo [bar](/url)*
<p><em>foo <a href="/url">bar</a></em></p>
-->

*foo [bar](/url)*

404 CONFORMING

<!--
*foo
bar*
<p><em>foo
bar</em></p>
-->

*foo
bar*

405 CONFORMING

<!--
_foo __bar__ baz_
<p><em>foo <strong>bar</strong> baz</em></p>
-->

_foo __bar__ baz_

406 CONFORMING

<!--
_foo _bar_ baz_
<p><em>foo <em>bar</em> baz</em></p>
-->

_foo _bar_ baz_

407 CONFORMING

<!--
__foo_ bar_
<p><em><em>foo</em> bar</em></p>
-->

__foo_ bar_

408 CONFORMING

<!--
*foo *bar**
<p><em>foo <em>bar</em></em></p>
-->

*foo *bar**

409 CONFORMING

<!--
*foo **bar** baz*
<p><em>foo <strong>bar</strong> baz</em></p>
-->

*foo **bar** baz*

410 CONFORMING

<!--
*foo**bar**baz*
<p><em>foo<strong>bar</strong>baz</em></p>
-->

*foo**bar**baz*

411 CONFORMING

<!--
*foo**bar*
<p><em>foo**bar</em></p>
-->

*foo**bar*

412 CONFORMING

<!--
***foo** bar*
<p><em><strong>foo</strong> bar</em></p>
-->

***foo** bar*

413 CONFORMING

<!--
*foo **bar***
<p><em>foo <strong>bar</strong></em></p>
-->

*foo **bar***

414 CONFORMING

<!--
*foo**bar***
<p><em>foo<strong>bar</strong></em></p>
-->

*foo**bar***

415 CONFORMING

<!--
foo***bar***baz
<p>foo<em><strong>bar</strong></em>baz</p>
-->

foo***bar***baz

416 CONFORMING

<!--
foo******bar*********baz
<p>foo<strong><strong><strong>bar</strong></strong></strong>***baz</p>
-->

foo******bar*********baz

417 CONFORMING

<!--
*foo **bar *baz* bim** bop*
<p><em>foo <strong>bar <em>baz</em> bim</strong> bop</em></p>
-->

*foo **bar *baz* bim** bop*

418 CONFORMING

<!--
*foo [*bar*](/url)*
<p><em>foo <a href="/url"><em>bar</em></a></em></p>
-->

*foo [*bar*](/url)*

419 CONFORMING

<!--
** is not an empty emphasis
<p>** is not an empty emphasis</p>
-->

** is not an empty emphasis

420 CONFORMING

<!--
**** is not an empty strong emphasis
<p>**** is not an empty strong emphasis</p>
-->

**** is not an empty strong emphasis

421 CONFORMING

<!--
**foo [bar](/url)**
<p><strong>foo <a href="/url">bar</a></strong></p>
-->

**foo [bar](/url)**

422 CONFORMING

<!--
**foo
bar**
<p><strong>foo
bar</strong></p>
-->

**foo
bar**

423 CONFORMING

<!--
__foo _bar_ baz__
<p><strong>foo <em>bar</em> baz</strong></p>
-->

__foo _bar_ baz__

424 CONFORMING

<!--
__foo __bar__ baz__
<p><strong>foo <strong>bar</strong> baz</strong></p>
-->

__foo __bar__ baz__

425 CONFORMING

<!--
____foo__ bar__
<p><strong><strong>foo</strong> bar</strong></p>
-->

____foo__ bar__

426 CONFORMING

<!--
**foo **bar****
<p><strong>foo <strong>bar</strong></strong></p>
-->

**foo **bar****

427 CONFORMING

<!--
**foo *bar* baz**
<p><strong>foo <em>bar</em> baz</strong></p>
-->

**foo *bar* baz**

428 CONFORMING

<!--
**foo*bar*baz**
<p><strong>foo<em>bar</em>baz</strong></p>
-->

**foo*bar*baz**

429 CONFORMING

<!--
***foo* bar**
<p><strong><em>foo</em> bar</strong></p>
-->

***foo* bar**

430 CONFORMING

<!--
**foo *bar***
<p><strong>foo <em>bar</em></strong></p>
-->

**foo *bar***

431 CONFORMING

<!--
**foo *bar **baz**
bim* bop**
<p><strong>foo <em>bar <strong>baz</strong>
bim</em> bop</strong></p>
-->

**foo *bar **baz**
bim* bop**

432 CONFORMING

<!--
**foo [*bar*](/url)**
<p><strong>foo <a href="/url"><em>bar</em></a></strong></p>
-->

**foo [*bar*](/url)**

433 CONFORMING

<!--
__ is not an empty emphasis
<p>__ is not an empty emphasis</p>
-->

__ is not an empty emphasis

434 CONFORMING

<!--
____ is not an empty strong emphasis
<p>____ is not an empty strong emphasis</p>
-->

____ is not an empty strong emphasis

435 CONFORMING

<!--
foo ***
<p>foo ***</p>
-->

foo ***

436 CONFORMING

<!--
foo *\**
<p>foo <em>*</em></p>
-->

foo *\**

437 CONFORMING

<!--
foo *_*
<p>foo <em>_</em></p>
-->

foo *_*

438 CONFORMING

<!--
foo *****
<p>foo *****</p>
-->

foo *****

439 CONFORMING

<!--
foo **\***
<p>foo <strong>*</strong></p>
-->

foo **\***

440 CONFORMING

<!--
foo **_**
<p>foo <strong>_</strong></p>
-->

foo **_**

441 CONFORMING

<!--
**foo*
<p>*<em>foo</em></p>
-->

**foo*

442 CONFORMING

<!--
*foo**
<p><em>foo</em>*</p>
-->

*foo**

443 CONFORMING

<!--
***foo**
<p>*<strong>foo</strong></p>
-->

***foo**

444 CONFORMING

<!--
****foo*
<p>***<em>foo</em></p>
-->

****foo*

445 CONFORMING

<!--
**foo***
<p><strong>foo</strong>*</p>
-->

**foo***

446 CONFORMING

<!--
*foo****
<p><em>foo</em>***</p>
-->

*foo****

447 CONFORMING

<!--
foo ___
<p>foo ___</p>
-->

foo ___

448 CONFORMING

<!--
foo _\__
<p>foo <em>_</em></p>
-->

foo _\__

449 CONFORMING

<!--
foo _*_
<p>foo <em>*</em></p>
-->

foo _*_

450 CONFORMING

<!--
foo _____
<p>foo _____</p>
-->

foo _____

451 CONFORMING

<!--
foo __\___
<p>foo <strong>_</strong></p>
-->

foo __\___

452 CONFORMING

<!--
foo __*__
<p>foo <strong>*</strong></p>
-->

foo __*__

453 CONFORMING

<!--
__foo_
<p>_<em>foo</em></p>
-->

__foo_

454 CONFORMING

<!--
_foo__
<p><em>foo</em>_</p>
-->

_foo__

455 CONFORMING

<!--
___foo__
<p>_<strong>foo</strong></p>
-->

___foo__

456 CONFORMING

<!--
____foo_
<p>___<em>foo</em></p>
-->

____foo_

457 CONFORMING

<!--
__foo___
<p><strong>foo</strong>_</p>
-->

__foo___

458 CONFORMING

<!--
_foo____
<p><em>foo</em>___</p>
-->

_foo____

459 CONFORMING

<!--
**foo**
<p><strong>foo</strong></p>
-->

**foo**

460 CONFORMING

<!--
*_foo_*
<p><em><em>foo</em></em></p>
-->

*_foo_*

461 CONFORMING

<!--
__foo__
<p><strong>foo</strong></p>
-->

__foo__

462 CONFORMING

<!--
_*foo*_
<p><em><em>foo</em></em></p>
-->

_*foo*_

463 CONFORMING

<!--
****foo****
<p><strong><strong>foo</strong></strong></p>
-->

****foo****

464 CONFORMING

<!--
____foo____
<p><strong><strong>foo</strong></strong></p>
-->

____foo____

465 CONFORMING

<!--
******foo******
<p><strong><strong><strong>foo</strong></strong></strong></p>
-->

******foo******

466 CONFORMING

<!--
***foo***
<p><em><strong>foo</strong></em></p>
-->

***foo***

467 CONFORMING

<!--
_____foo_____
<p><em><strong><strong>foo</strong></strong></em></p>
-->

_____foo_____

468 CONFORMING

<!--
*foo _bar* baz_
<p><em>foo _bar</em> baz_</p>
-->

*foo _bar* baz_

469 CONFORMING

<!--
*foo __bar *baz bim__ bam*
<p><em>foo <strong>bar *baz bim</strong> bam</em></p>
-->

*foo __bar *baz bim__ bam*

470 CONFORMING

<!--
**foo **bar baz**
<p>**foo <strong>bar baz</strong></p>
-->

**foo **bar baz**

471 CONFORMING

<!--
*foo *bar baz*
<p>*foo <em>bar baz</em></p>
-->

*foo *bar baz*

472 CONFORMING

<!--
*[bar*](/url)
<p>*<a href="/url">bar*</a></p>
-->

*[bar*](/url)

473 CONFORMING

<!--
_foo [bar_](/url)
<p>_foo <a href="/url">bar_</a></p>
-->

_foo [bar_](/url)

474 CONFORMING

<!--
*<img src="foo" title="*"/>
<p>*<img src="foo" title="*"/></p>
-->

*<img src="foo" title="*"/>

475 CONFORMING

<!--
**<a href="**">
<p>**<a href="**"></p>
-->

**<a href="**">

476 CONFORMING

<!--
__<a href="__">
<p>__<a href="__"></p>
-->

__<a href="__">

477 CONFORMING

<!--
*a `*`*
<p><em>a <code>*</code></em></p>
-->

*a `*`*

478 CONFORMING

<!--
_a `_`_
<p><em>a <code>_</code></em></p>
-->

_a `_`_

479 CONFORMING

<!--
**a<http://foo.bar/?q=**>
<p>**a<a href="http://foo.bar/?q=**">http://foo.bar/?q=**</a></p>
-->

**a<http://foo.bar/?q=**>

480 CONFORMING

<!--
__a<http://foo.bar/?q=__>
<p>__a<a href="http://foo.bar/?q=__">http://foo.bar/?q=__</a></p>
-->

__a<http://foo.bar/?q=__>

