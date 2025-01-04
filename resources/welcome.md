<mtx>
  <viewer>
    <track_page>0</track_page>
  </viewer>
</mtx>
## Welcome to MDVIEW MTX

- Press `[F1]` or start a blank text search to display this page in the future.
- [Learn markdown] in 60 seconds!
- Full [online manual].

**MDVIEW MTX** is an [open source] graphical Markdown viewer and converter.
Development occurs on the [MDVIEW MTX project page], where users can [report
issues] and contribute to the project. Below is a brief help guide for the
program.

&nbsp;

The **home page directory** is the directory in which the home page is located.
The home page is the initial file that the viewer displays. Clicking the home
icon reloads the home page. Clicking the arrow icons moves through the trail of
displayed pages. When you click to go to, or away from, a page, the trail marks
the current cursor position then changes page. As you move through trail pages,
the cursor returns to saved marks.

The **search input field** offers two search functions:
* **Document Search**: Click the left icon or press the `[Enter]` key.
* **Page Search**: Click the right icon or press `[Control]` + `[f]`.

**Document Search** looks in the home page directory for files that include
**any** of the search terms, and produces a list of links to the matching
documents. For example, `red apples` finds files matching `apples` or `red` or
both.

**Page Search** looks in the current page for the next match of the search words
**strung together**. For example, `red apples` does not match `apples` or `red`,
it only matches `red apples`.

In Document Search, terms that include spaces must be quoted. Thus, `"term 1"`
and `term2` are two search terms, the first one consisting of two words; but
`term 1` and `term2` are three search terms, each one consisting of a word.

In Page Search, all the words you enter are treated as a single search term, and
quotation marks are ignored. To search for text that includes quotes, you need
to escape the quotes with a backslash. For example, searching for `term` is the
same as entering `"term"`, but to search for the literal text `"term"`, you
should enter `\"term\"`.

### Keyboard shortcuts

Using keyboard shortcuts provides fast navigation inside the page and through
search results. In addition to the Arrow, Page, Home and End keys and their
Shift/Control/Alt variants, the following special shortcuts enhance navigation.

Symbols: `               ` `A`=`[Alt]` `C`=`[Control]` `S`=`[Shift]`  

Shortcut   | Description
-----------|----------------------------------------------------
`[A-s]`    | Set focus on the search field to enter terms
`[Enter]`¹ | Activate search within the home page folder
`[Tab]`¹   | Move focus out of the search field
 &nbsp;    | &nbsp;
`[C-f]`    | Activate search or find the next match in the page
`[C-b]`    | Find the previous match in the page
&nbsp;     | &nbsp;
`[A-b]`    | Navigate back to the previous page
`[A-f]`    | Navigate forward to the next page
`[A-h]`    | Reload the home page
 &nbsp;    | &nbsp;
`[S-C-b]`  | Find the previous link in the page
`[S-C-f]`  | Find the next link in the page
`[Enter]`² | Open the link at the current cursor location
 &nbsp;    | &nbsp;
`[C-e]`    | Open the current page in the default text editor
`[A-p]`³   | Preview the current page in the default browser

[¹] If the search field is focused.  
[²] At the text cursor position if the text view area is focused.  
> Don't be surprised if you use `[A-s]` `[C-f]` to find a certain link label,
but nothing happens when you press `[Enter]` to follow the link. This occurs
because the focus is still on the search field. To successfully follow the link,
press `[Tab]` first to shift the focus away from the search field, allowing you
to activate the link with `[Enter]`.  

[³] Use command-line option `--html-css=N` to prettify the preview page.  

### Extensions

Mdview features extensions that enhance its functionality.
Run `mdview -h` to see the list of available extensions,
and refer to the [Extensions online page].

### Configuration

Mdview does not have a configuration file. For advanced
needs refer to the [Configuration online page].

### Links

* MDVIEW MTX project page <https://github.com/step-/mdview>
* Online manual <https://github.com/step-/mdview/wiki>
* Reporting issues <https://github.com/step-/issues>
* Learn markdown: <https://commonmark.org/help>
* Mdview supports CommonMark: <https://commonmark.org>
* Markdown on Wikipedia: <https://wikipedia.org/wiki/Markdown>
* Open source on Wikipedia <https://en.wikipedia.org/wiki/Open_source>

[MDVIEW MTX project page]: <https://github.com/step-/mdview>
[online manual]: <https://github.com/step-/mdview/wiki>
[report issues]: <https://github.com/step-/issues>
[Learn markdown]: <https://commonmark.org/help>
[CommonMark]: <https://commonmark.org>
[open source]: <https://en.wikipedia.org/wiki/Open_source>
[fontconfig]: <https://en.wikipedia.org/wiki/Fontconfig>
[Configuration online page]: <https://github.com/step-/mdview/wiki/Configuration>
[Extensions online page]: <https://github.com/step-/mdview/wiki/Extensions>
