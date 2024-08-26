### MDVIEW MTX Readme

**MDVIEW MTX** is a graphical Markdown viewer and CLI converter. It supports
the [CommonMark] specification version [0.31.2] via [MD4C], a C Markdown parser.

* Home: <http://github.com/step-/mdview>
* Wiki: <http://github.com/step-/mdview/wiki>
* Issue tracker: <http://github.com/step-/mdview/issues>

### Dependencies

- Unix-like OS (development and testing takes place on Linux).
- GTK+ 2 or GTK+ 3 and their dependencies.
- Pango >= 1.50, Glib >= 2.66.
- GNU make or compatible make program, and pkg-config.

### Building

For GTK+ 3 just run `make`. See the Makefile for build options. After building,
copy the self-contained "mdview" executable file to the installation directory.

### License

GNU General Public License Version 2.

### Project History

MDVIEW MTX is a continuation of the [mdview3] project. The MTX part of the
name comes from `mtx`, the three-letter prefix that I use for class and
other names in the source code. MTX does not mean anything in particular.

### Links

* Mdview3: <http://chiselapp.com/user/jamesbond/repository/mdview3>
* CommonMark: <https://commonmark.org>
* CommonMark version 0.31.2: <https://spec.commonmark.org/0.31.2>
* MD4C: <https://github.com/mity/md4c>

[mdview3]: <http://chiselapp.com/user/jamesbond/repository/mdview3>
[CommonMark]: <https://commonmark.org>
[0.31.2]: <https://spec.commonmark.org/0.31.2>
[MD4C]: <https://github.com/mity/md4c>
