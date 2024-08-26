## Welcome to MDVIEW MTX

This page is shown when you activate an empty text search or pressed `[F1]`.

**MDVIEW MTX** is an [open source] graphical [CommonMark] viewer, and a
CLI converter. The [MDVIEW MTX project page] is where development takes
place, and where you can [report an issue] and contribute to the project.

[Learn markdown] in 60 seconds.

### Condensed help

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

Shortcuts are likely the fastest way to navigate inside the page and through
search results.

Symbols: `               ` `A`=`[Alt]` `C`=`[Control]` `S`=`[Shift]`  

Shortcut   | Action        | Description
-----------| --------------|----------------------------------------------------
`[A-s]`    | Focus search  | Set focus on the search field to enter terms
`[Enter]`¹ | Search all    | Activate search through all documents
`[Tab]`¹   | Focus out     | Move focus out of the search field
 <br>      | <br>          | <br>
`[C-f]`    | Find start    | Activate search in the page
`[C-f]`    | Find forward  | Find next match in the page
`[C-b]`    | Find backward | Find previous match in the page
 <br>      | <br>          | <br>
`[A-b]`    | Go back       | Show previous page in navigation trail
`[A-f]`    | Go forward    | Show next page in navigation trail
`[A-h]`    | Go home       | Reload home page
 <br>      | <br>          | <br>
`[S-C-b]`  | Link back     | Find previous link in the page
`[S-C-f]`  | Link forward  | Find the next link in the page
`[Enter]`² | Follow link   | Open link at cursor location
 <br>      | <br>          | <br>
`[C-e]`    | Edit          | Open the current file in the default text editor

[¹] Only when the search field is focused.  
[²] At the text cursor position, and only when the text view area is focused.
So, if you pressed `[A-s]` to enter the name of a link and then used `[C-f]`
once or more until the link was reached, finally you pressed `[Enter]` to follow
the link but nothing happened. Why? The reason for this is that input focus was
still in the search field. To follow the link in this case, you need to press
`[Tab]` first. This moves the focus away from the search field and allows you to
activate the link with `[Enter]`.

Continue to the [online manual].

## Links

* MDVIEW MTX project page <https://github.com/step-/mdview>
* Online manual <https://github.com/step-/mdview/wiki>
* Report an issue <https://github.com/step-/issues>
* Learn markdown: <https://commonmark.org/help>
* CommonMark: <https://commonmark.org>
* Markdown on Wikipedia: <https://wikipedia.org/wiki/Markdown>
* Open source on Wikipedia <https://en.wikipedia.org/wiki/Open_source>

[MDVIEW MTX project page]: <https://github.com/step-/mdview>
[online manual]: <https://github.com/step-/mdview/wiki>
[report an issue]: <https://github.com/step-/issues>
[Learn markdown]: <https://commonmark.org/help>
[CommonMark]: <https://commonmark.org>
[markdown]: <https://en.wikipedia.org/wiki/Markdown>
[open source]: <https://en.wikipedia.org/wiki/Open_source>

