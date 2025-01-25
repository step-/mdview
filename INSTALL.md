# Installation Overview

The meson-based build can compile and install a release target, including any
available translation data.

For development and testing use the included Makefile, which can build, but
not install, the project. See § [Building using the included Makefile].

## GTK+-3 runtime dependencies

Mdview depends on the following libraries:
libgtk-3-0 (>= 3.24.33), libpango-1.0-0 (>= 1.50.6), libglib2.0-0 (>= 2.72.4).

Dependencies satisfy the package versions
that the following distributions provide:

|                       |             |                |              |
|-----------------------|-------------|----------------|--------------|
|───────────────────────|─────────────|────────────────|──────────────|
| Debian package name   | libgtk-3-0  | libpango-1.0-0 | libglib2.0-0 |
|───────────────────────|─────────────|────────────────|──────────────|
| **12 bookworm**       | 3.24.38     | 1.50.12        | 2.74.6       |
| 13 trixie (testing)   | 3.24.43     | 1.55.0         | 2.82.4       |
| sid  (unstable)       | 3.24.43     | 1.55.0         | 2.82.4       |
|───────────────────────|─────────────|────────────────|──────────────|
| Ubuntu package name   | libgtk-3-0  | libpango-1.0-0 | libglib2.0-0 |
|───────────────────────|─────────────|────────────────|──────────────|
| **22.04 (LTS) Jammy** | **3.24.33** | **1.50.6**     | **2.72.4**   |
| Noble                 | 3.24.41     | 1.52.1         | 2.80.0       |
| Oracular              | 3.24.43     | 1.54.0         | 2.82.1       |
| Plucky                | 3.24.43     | 1.55.0         | 2.82.4       |
|───────────────────────|─────────────|────────────────|──────────────|
| Fatdog64 package name | gtk3        | pango          | glib         |
|───────────────────────|─────────────|────────────────|──────────────|
| **903**               | 3.24.42     | 1.50.12        | 2.74.5       |

### GTK+-2 dependencies

You should use the latest GTK+-2 version available for your distribution.
On Fatdog64 Linux, I use gtk2 2.24.33.

## Building and installing using meson

Meson's default installation prefix is `/usr/local`.

	[CC=gcc] meson setup build      # set CC to disable ccache autodetection
	meson compile -C build
	meson install -C build

## Building using the included Makefile

	GTK+-3 release build                : make
	GTK+-2 release build                : make GTK=2
	standard debug build                : make DEBUG=1
	run tests in a debug build          : make TEST=1 [TEST_GROUP]
	custom build                        : make [VARS] [DEBUG=1] [TEST=1]

The top Makefile sets LOCALEDIR equals to `/usr/local/share/locale`.
You can change it by passing, e.g., `LOCALEDIR=/usr/share/locale` to the
make command.

The standard build runs `$(CC)` passing default release options.
If the make invocation includes `DEBUG=1` the Makefile appends default
debug options to the set of release options, overriding the release intent.
`TEST=1` appends more debug options enabling test hooks in the binary file.
See the main Makefile for the available `TEST_GROUP` targets.

Make invokes `$(CC)` passing `CPPFLAGS`, `DEBUG_CPPFLAGS`, `CFLAGS` and
`DEBUG_CFLAGS`in this order. `DEBUG=1` and `TEST=1` append to the `DEBUG_*`
variables overriding their values including those set on the command line.

Make runs the following non-standard tools, which you will need to install
on your system if you intend to modify and recompile the source code:

* fccf - <https://github.com/p-ranav/fccf>.

## Building translations

To update the Gettext POT template in directory `./po`:

    meson compile -C build mdview-pot

To update translations under directory `build/po`:

    meson compile -C build      # don't also append target mdview-update-po

## Building and installing documentation

If the help2man(1) command is available, meson will build and install the
manual page.
