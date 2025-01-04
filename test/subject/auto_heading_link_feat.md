Auto-heading links
==================

The auto-heading link feature automatically constructs a CommonMark link
reference definition for the display text of each heading in the document. Then
authors can add a _regular link_ to jump to the heading. The link destination
must start with `#` (same as the in-page links of HTML documents). If the
title includes spaces, wrap the destination with matching pointed brackets.

Example of a regular link:

    # This heading title
                               ┌── destination ──┐
    [Jump to "Heading Title"](<#This heading title>)

In addition to jumping to the heading with a regular link, one can jump with a
_reference link_ as the [specification][CommonMark reference link] explains.

Examples of reference links:

    [This heading title]
    [Jump to title][This heading title]

The reference link destination must exactly match
the heading text, omitting any leading `#`.

**Important:**
Heading text collisions are not managed. If two headings
share the same display text, all reference links to that text
will direct to the first matching heading in the document.

[CommonMark reference link]: <https://spec.commonmark.org/0.31.2/#reference-link>

------------------------------------------------------------------------------

## Auto-heading link tests

* [Jump to "Auto-heading links"](<#Auto-heading links>)
* [Jump to "TITLE"][TITLE]
* [Jump to "ATX 1"][ATX 1]
* [Jump to "ATX 2 🙂"](<#ATX 2 &#x1F642;>)
* [Jump to "ATX 2 🙂 🙂"][ATX 2 🙂 🙂]
* [Jump to "SETEXT 1 !"](<#SETEXT 1 !>)
* [Jump to "SETEXT 2"][SETEXT 2]
* [Jump to "***Strong Emphasis***"][Strong Emphasis]
* [Jump to "# this title starts and ends with #"](<## this title starts and ends with #>)
* [Jump to "SETEXT Z +SETEXT Z 🙂"][SETEXT Z +SETEXT Z 🙂] (multiline title)
* [Jump to "SETEXT BR+SETEXT"][SETEXT BR+SETEXT] (multiline title with a hard line break)

------------------------------------------------------------------------------

# TITLE

atx 1 title above.

------------------------------------------------------------------------------

# ATX 1

atx 1 title above.

------------------------------------------------------------------------------

## ATX 2 &#x1F642;

atx 2 title above - written as `ATX 2 &#x1F642;`.

------------------------------------------------------------------------------

## ATX 2 🙂 🙂

atx 2 title above - written as `ATX 2 🙂 🙂`.

------------------------------------------------------------------------------

SETEXT 1 !
==========

setext 1 title above.

------------------------------------------------------------------------------

  SETEXT 2    
------------

setext 2 title above - written as &#x2423;&#x2423;`SETEXT 2`&#x2423;&#x2423;&#x2423;&#x2423;

_Leading (up to three) and trailing spaces are stripped from the title._

------------------------------------------------------------------------------

#### ***Strong Emphasis***

atx 4 title above - written as `*** Strong Emphasis***`.

_The small-caps font variant and the bold, italics text styles do not affect how
one writes the link destination: `[Strong Emphasis]`._ [Try][Strong Emphasis].

------------------------------------------------------------------------------

# Corner cases

The following titles can be processed for heading links but be mindful of
the necessary Markdown syntax.

### \# this title starts and ends with # ############

### (brackets are OK()

------------------------------------------------------------------------------

# Unfriendly corner cases

Titles that include, `[`, `<`, `>`, `]`, `\\` and
`&` are not processed for heading links. This measure
avoids generating invalid Markdown link reference syntax.

### b [
### c <
### d >
### e ]
### g \\
### h &

------------------------------------------------------------------------------

## The section below isn't supposed to contain titles!

------------------------------------------------------------------------------

> quote isn't a title
===========

* list item isn't a title
===========

- list item isn't a title
===========

1. ordered list item isn't a title
====================

```
# code block isn't a title
```

------------------------------------------------------------------------------

## Even links to multiline SETEXT headings are supported!

*like*

```
    SETEXT Z
    +SETEXT Z 🙂
    ============
```

SETEXT Z
+SETEXT Z 🙂
============

*and*

```
    SETEXT BR  
    +SETEXT
    -----------
```

SETEXT BR  
+SETEXT
-----------

Note that the link to the title **will not** contain the hard line break.

