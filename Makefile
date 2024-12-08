# Depends: Pango >= 1.50, Glib >= 2.68 (g_string_replace)

# | Pango  | GLib | Debian               | Ubuntu                   |
# |--------|------|----------------------|--------------------------|
# | 1.50.6 | 2.66 | Debian 11 (Bullseye) | Ubuntu 21.04 (Hirsute)   |
# | 1.54.0 | 2.68 | Debian 12 (Bookworm) | Ubuntu 22.04 LTS (Jammy) |

# GNU Make
# Read INSTALL.md for build instructions.

export PACKAGE_NAME    = mdview
export PACKAGE_VERSION = 2024.12.08
export PACKAGE_URL     = http://github.com/step-/mdview

export GETTEXT_PACKAGE = $(PACKAGE_NAME)
export LOCALEDIR       = /usr/local/share/locale

ifeq ($(wildcard .git),.git)
	PACKAGE_VERSION += $(shell git describe --all --match master --match main --dirty)
endif

GTK?=3
GTK_CFLAGS::=$(shell pkg-config --cflags gtk+-$(GTK).0)
GTK_LIBS::=$(shell pkg-config --libs gtk+-$(GTK).0)

###############################################################################
#  RELEASE #
############

# Disable GTypeDebugFlags and GTimeVal deprecations, might be overkill.
CPPFLAGS+=-DGTK_DISABLE_DEPRECATED

# Increase compiler warnings. Orthogonal to disabling GTK deprecations.
CFLAGS+=-Wall -Wextra -Wshadow

# Define this symbol to insert a blank line before the closing HTML tag of
# a fenced code block, as done by the `cmark` command. By default, this
# symbol is set undefined so authors can decide whether to add the extra line
# explicitly. Run-time option `--cm-block-end` forces the renderer to add extra
# lines regardless of this symbol.
CPPFLAGS+=-UCOMMONMARK_FENCED_CODEBLOCK_LINE_ENDING

# Disable code assertions for release build.
CPPFLAGS+=-DG_DISABLE_ASSERT

# Compiler optimizations for release build.
CFLAGS+=-O3

# Catch direct access to GObject fields.
CPPFLAGS+=-DG_SEAL_ENABLE
# Warn instead of bailing out.
#CFLAGS+=-DG_SEAL_NO_ERRORS
###############################################################################

###############################################################################
#  DEBUG  #
###########

### DEBUG_CPPFLAGS ###

# Define build flag OPT_PANGO to enable command-line option `--pango`, which is
# a requirement of the "validate_pango_markup" Makefile target.
#DEBUG_CPPFLAGS += -DOPT_PANGO

# Define build flag OPT_MARKUP to enable command-line option `--markup=FILE`,
# which allows passing the parser a file obtained with `--pango`.
#DEBUG_CPPFLAGS += -DOPT_MARKUP

# Define build flag OPT_EXIT_TEST to enable command-line option `--exit-test`,
# which allows exiting the viewer for test automation.
#DEBUG_CPPFLAGS += -DOPT_EXIT_TEST

# Add -DMTX_DEBUG=Level for general diagnostics. Level, a positive integer,
# can take the following values:
# 0 compile debugging functions;
# 1 print elapsed times and parser unit tally
# 2 dump input markdown, protected output and code table;
# 3 dump parser queues.
#DEBUG_CPPFLAGS +=-DMTX_DEBUG=0

# Add -DMTX_TEXT_VIEW_DEBUG=Level for MtxTextView diagnostics. Level, a
# positive integer, can take the following values:
# 1 to print the table of link destinations;
# 2 to print select Pango attributes;
# 3 to visualize blockquote stops;
# 4 to print indent and margin values.
#DEBUG_CPPFLAGS +=-DMTX_TEXT_VIEW_DEBUG=0

# Add -DVIEWER_DEBUG=0 to print document navigation trail to stdout.
#DEBUG_CPPFLAGS += -DVIEWER_DEBUG=0



### DEBUG_CFLAGS ###

# Collect gprof profiling data
#DEBUG_CFLAGS  += -pg
###############################################################################

###############################################################################
# Changing DEBUG_CPPFLAGS and DEBUG_CFLAGS after this block requires 'override'.
# -DDEBUG only serves as a visual clue for make's $(CC) invocation trace.
ifeq ($(DEBUG),1)
override DEBUG_CPPFLAGS += -UG_DISABLE_ASSERT
override DEBUG_CFLAGS   += -DDEBUG -O0 -ggdb3
endif
ifeq ($(TEST),1)
override DEBUG_CPPFLAGS += -DOPT_PANGO -DOPT_MARKUP -DOPT_EXIT_TEST
endif
###############################################################################


LIBS ::= $(GTK_LIBS)

ALL_FLAGS ::= $(GTK_CFLAGS) $(CPPFLAGS) $(DEBUG_CPPFLAGS) $(CFLAGS) $(DEBUG_CFLAGS)

all: sub-config sub-resources sub-src

sub-config:
	@$(MAKE) -C src ALL_FLAGS="$(ALL_FLAGS)" LIBS="$(LIBS)" mtxversion.h

sub-resources:
	@$(MAKE) -C resources SUB_CPPFLAGS="$(CPPFLAGS) $(DEBUG_CPPFLAGS)"

sub-src:
	@$(MAKE) -C src ALL_FLAGS="$(ALL_FLAGS)" LIBS="$(LIBS)"

clean:
	@for p in resources src; do $(MAKE) -C $$p $@; done

test: all test-unattended test-validate-pango

define test_opt_pango
if ! mdview --text --pango /dev/null 2> /dev/null; then \
	echo 'Target $@: build mdview with DEBUG_CPPFLAGS+=-DOPT_PANGO.' >&2; \
	false; \
fi
endef

test-unattended: all
	@$(test_opt_pango)
	@test/run_unattended_tests.sh

test-validate-pango: all
	@$(test_opt_pango)
	@test/validate_pango_markup.sh

install: all
	@echo "This makefile is intended for development only." >&2; \
	echo "Please build and install with meson (see INSTALL.md)." >&2; \
	false

### build distribution package
package: clean
	@echo "TODO $@"; false

.PHONY: all clean install package sub-config sub-resources sub-src \
	test test-unattended test-validate-pango-markup
