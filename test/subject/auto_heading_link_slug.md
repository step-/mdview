Auto-heading link anchor
------------------------

Given the heading "The heading", the auto-heading link feature
adds the following in-page anchors at the heading's line:

| Anchor         | Referenced by         |
|----------------|-----------------------|
| `#The heading` | verbatim heading text |
| `#the_heading` | snake-case [slug]     |
| `#the-heading` | kebab-case [slug]     |

Therefore the linked heading can be reached equivalently as follows:

| Link                               | Markdown                           |
|------------------------------------|------------------------------------|
| [The heading]                      | `[The heading]`                    |
| [To The heading][The heading]      | `[To The heading][The heading]`    |
| [To the_heading](<#the_heading>)   | `[To the_heading](<#the_heading>)` |
| [To the-heading](<#the-heading>)   | `[To the-heading](<#the-heading>)` |
| [To THE-Heading](<#THE-heading>) ¹ | `[To THE-heading](<#THE-heading>)` |
| [To the-HEADing](<#the-HeAdinG>) ¹ | `[To the-HEADing](<#the-HeAdinG>)` |

Try each of the links above, they should jump to **The heading** below.
Then click "back to section top" to clear the highlight and try another link.
1. _Note that the last two links in the table use mixed
   case. Technically, they should not work because slugs are
   required to be in lowercase. But mdview is forgiving!_

### The heading

[back to section top](<#auto-heading-link-anchor>)


Slug compatibility
------------------

Mdview implements kebab-case slugs according to [pandoc]'s specification of
[auto_identifiers] and [ascii_identifiers] extensions. To demonstrate such is
the case, the Table of Contents below was created with pandoc, passing the 9
headings beneath the ToC as markdown input:

```sh
pandoc -f markdown-smart+auto_identifiers+ascii_identifiers -t commonmark --toc -s << \EOF | grep ^-
    copy the headings here
EOF
```

Clicking on the heading links below should jump to their respective
headings. However, items \#2, \#8, and \#9 cannot be accessed this
way. The slug creation rules for items 2 and 8 result in empty slugs,
making them unreachable, while item 9 is affected by the mitigations
described [here](auto_heading_link_bogus.md).

- [1 Maître d'hôtel. !](#maitre-dhotel.)
- [2 中文标题. !](#section)
- [3 é è ê ë ç á à â ä í ì î ï ó ò ô ö ú ù û ü. !](#e-e-e-e-c-a-a-a-a-i-i-i-i-o-o-o-o-u-u-u-u.)
- [4 ALLCAPS. !](#allcaps.)
- [5 'I ♥ Dogs'.; !](#i-dogs.)
- [6 ' Déjà Vu! '.; !](#deja-vu-.)
- [7 'fooBar 123 $#%'.; !](#foobar-123-.)
- [8 'я люблю единорогов'.; !](#section-1)
- [9 'a-b_c D--E\_\_F'... !](#a-b_c-d--e__f...)

### 1 Maître d'hôtel. !
_slug `maitre-dhotel.`_

### 2 中文标题. !
_empty slug_ --
To jump to this heading use the reference link [2 中文标题. !] syntax.

### 3 é è ê ë ç á à â ä í ì î ï ó ò ô ö ú ù û ü. !
_slug `e-e-e-e-c-a-a-a-a-i-i-i-i-o-o-o-o-u-u-u-u.`_

### 4 ALLCAPS. !
_slug `allcaps.`_

### 5 I ♥ Dogs.; !
_slug `i-dogs.`_

### 6   Déjà Vu!  .; !
`deja-vu-.`_

### 7 fooBar 123 $#%.; !
_slug `foobar-123-.`_

### 8 я люблю единорогов.; !
_empty slug_ --
To jump to this heading use the reference link [8 я люблю единорогов.; !] syntax.

### 9 a-b_c D--E__F... !
_slug `a-b_c-d--e__f...`_ --
This heading cannot be jumped to due to the aforementioned mitigations.

[slug]: https://en.wikipedia.org/wiki/Clean_URL#Slug
[pandoc]: https://pandoc.org
[auto_identifiers]: https://pandoc.org/MANUAL.html#extension-auto_identifiers
[ascii_identifiers]: https://pandoc.org/MANUAL.html#extension-ascii_identifiers
