# DESCRIPTION

**Auto-code is an mdview extension**
It is enabled by default, and it can be disabled with option `--no-ext`.

An **auto-code** is a word that is rendered as a code span even though it isn't surrounded by grave accents.
The word is at least 4 bytes long. Inner white space must be escaped with backslash.


# SUPPORTED AUTO-CODES

**absolute path** -- any word that starts with `/` and is at least four bytes long.
- _1 byte = 1 character for the ASCII and 8-bit character sets_
- _2 to 4 bytes = 1 character for other unicode characters_

* /123 /naïve /😃
* "/dir/file" '/dir/file' /file.. "/file," '/file:' /file; /file! /file?
* /dir/ "/dir/" (a terminating slash is included)
* but not /12 /1 / /§ (too short: 3, 2, 1 and 2 bytes respectively)
* but not /12/ "/12/" (a terminating slash is _not_ counted in the byte count)
* but not name: "/file name" (two words: «/file» and «name»; see [BACKSLASH TESTS] for a work-around).

**dot relative path** -- any word that starts with `./` or `../` and is at least four bytes long.

* ../file ../../dir/
* ./file ./dir/file
* ./.bashrc

**special file names**

file names ending with extensions ".patch" or ".diff":

* file.patch f.diff "f.diff"
* f.diff, f.diff.
* but not .patch .diff ".diff" (need a file _name_, just the extension is not enough)
* but not f.diff.old (does not end with the right extension)
* Caveat: "f.diff."

**URI**

The auto-code and permissive auto-link extensions contend for free-standing URIs.
Permissive auto-link wins unless disabled with option `--no-permlink`.

* http:// https:// ftp://
* http://www https://www.example ftp://www.example.com
* wrapped: "http://example.com", (https://www.example.com)
* http://a.com/ http://a.com? http://a.com?v= http://a.com?v=.

Note that angle brackets indicate an **auto-link** instead of an auto-code.
For example, `<`\https://www.example.com`>` turns into <https://www.example.com>.

**issue id (bugzilla, gh)** -- "#" followed by an ASCII word

* #123 #abc
* not \\#123 \\#abc (backslash escapes)

**function name** -- ASCII identifier that ends with "()"

* foo() foo(); foo_bar() f9()
* with backslash before underscore: \_foo() - without: _foo()
* but not () "()" (no identifier)
* but not 32() (32 not an identifier)

**uppercase identifier** -- uppercase ASCII identifier that includes at least one underscore

* XDG_CONFIG_HOME and \_Z\_DATA
* $XDG_CONFIG_HOME also if it starts with "$"
* with all underscores escaped: \_ABC \_ABC\_
* with the first but not the last underscore escaped: \_ABC_
* but not DISPLAY (no underscore), $HOME (no underscore)
* but not _HOME_ (italics)

**email address** -- ASCII word, must not start with "@" or ".", must include one "@" and at least one interior "."

The auto-code and permissive auto-link extensions contend for free-standing email addresses.
Permissive auto-link wins unless disabled with option `--no-permlink`.

* a@b.com
* not @ @ID macro@ @@@@@@@@@

# FORMATTING

Exterior formatting - italics, bold, strikethrough - is applied to auto-codes.

- _/root_
- _/italics_ **/bold** ~/strikethrough~
- ***/3/star/bold-italics / /*** ___/ / /3/under/bold-italics___
-  ***~bold italics strikethrough~***
- _#123_ _#abc_
**#123** **#abc**
- _http:// https:// ftp://_
**http:// https:// ftp://**
- _file.patch_ _file.diff_
**file.patch** **file.diff**
- _foo()_
**foo()**
- _XDG_CONFIG_HOME_ _\_Z_DATA_ _$XDG_CONFIG_HOME_
**XDG_CONFIG_HOME** **\_Z_DATA** **$XDG_CONFIG_HOME**
- _XDG_CONFIG_HOME \_Z_DATA $XDG_CONFIG_HOME_
**XDG_CONFIG_HOME \_Z_DATA $XDG_CONFIG_HOME**
- _a@b.com_
**a@b.com**

## BACKTICK TESTS

Below explicit backticks wrap code chunks therefore auto-code isn't applied.

* `cp /tmp/a /tmp/b`
* `#123 and #456`
* `_ABC_, _DEF and _XYZ`

Exterior formatting is applied to explicit code chunks:

* two italics words and a bold URI: _`foo() user@email.com`_ **`https://mdview.com`**

Mixing code chunks, auto-codes and formatting works:

* `cp /code` /auto-code
* _`ln -s`_ ***/auto-code***

## BACKSLASH TESTS

Note that quotes must be placed outside emphasis to enable smart quote replacement.

* Replaced:
#123
_#123x
"_#123_"

* Replaced including white space:
/file\ name

* Not replaced (one backslash before foo)
\foo()

* Not replaced (special: double backslash before #):
\\#123
_\\#123_
"_\\#123_"
