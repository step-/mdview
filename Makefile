# Depends: Pango >= 1.50, Glib >= 2.66
# GNU Make

# For GTK+-3 release build : make
# For GTK+-2 release build : GTK=2 make
# For debug build          : DEBUG=-DDEBUG make
#	more debugging options can be uncommented in this Makefile

.PHONY: all clean subdirs test test-unattended test-validate-pango-markup

SUBDIRS = resources

GTK?=3
GTK_CFLAGS::=$(shell pkg-config --cflags gtk+-$(GTK).0)
GTK_LIBS::=$(shell pkg-config --libs gtk+-$(GTK).0)

# Disable GTypeDebugFlags and GTimeVal deprecations, might be overkill.
CFLAGS+=-DGTK_DISABLE_DEPRECATED

# Increase compiler warnings. Orthogonal to disabling GTK deprecations.
CFLAGS+=-Wall -Wextra -Wshadow

# Define this symbol to insert a blank line before the closing HTML tag of
# a fenced code block, as done by the `cmark` command. By default, this
# symbol is set undefined so authors can decide whether to add the extra line
# explicitly. Run-time option `--cm-block-end` forces the renderer to add extra
# lines regardless of this symbol.
CFLAGS+=-UCOMMONMARK_FENCED_CODEBLOCK_LINE_ENDING

# Disable code assertions for release build.
CFLAGS+=-DG_DISABLE_ASSERT

# Compiler optimizations for release build.
CFLAGS+=-O3

# Catch direct access to GObject fields.
CFLAGS+=-DG_SEAL_ENABLE
# Warn instead of bailing out.
#CFLAGS+=-DG_SEAL_NO_ERRORS

ifdef DEBUG

DEBUG+=-O0 -g -UG_DISABLE_ASSERT
# Add -DMTX_DEBUG=0 to enable command-line option `--pango`, which is
# a requirement of the "validate_pango_markup" Makefile target.
# Increase MTX_DEBUG level for more diagnostics:
# 1 (reserved): (developer) suggested as the level of mtx_dbg_ functions;
# 2 to dump cooked markdown and reference tables; 3 to dump parser queues.
#DEBUG+=-DMTX_DEBUG=0
# Add -DMTX_TEXT_VIEW_DEBUG=0 to enable option `--markup=FILE`.
# Increase MTX_TEXT_VIEW_DEBUG level for more diagnostics:
# 1 to print select Pango attributes; 2 to visualize blockquote stops;
# 3 to print indent and margin values.
#DEBUG+=-DMTX_TEXT_VIEW_DEBUG=0
# Add -DVIEWER_DEBUG=0 to print document navigation trail to stdout
#DEBUG+=-DVIEWER_DEBUG=0

endif

CFLAGS+=$(GTK_CFLAGS) $(DEBUG) -Isrc

LIBS ::= $(GTK_LIBS)

SRC ::= \
	main.c \
	mtxviewer.c \
	mtxtextview.c \
	entity.c \
	md4c.c \
	mtxrender.c \
	mtx.c \
	mtxcmm.c

INCL ::= \
	mtxversion.h \
	mtxcolor.h \
	mtxstylepango.h \
	mtxviewer.h \
	mtxtextview.h \
	mtxtextviewprivate.h \
	entity.h \
	md4c.h \
	mtxrender.h \
	mtx.h \
	mtxcmm.h \
	mtxcmmprivate.h \
	mtxdbg.h

RES_DIR ::= resources

RES_SRC ::= $(RES_DIR)/all.c

all: subdirs mdview

mdview: $(SRC) $(INCL) Makefile $(RES_DIR)/all.gresource
	$(CC) $(SRC) $(RES_SRC) -o mdview $(CFLAGS) $(LIBS)

subdirs:
	@for p in $(SUBDIRS); do $(MAKE) -C $$p; done

clean:
	@for p in $(SUBDIRS); do $(MAKE) -C $$p $@; done
	$(RM) -v mdview

test: all test-unattended test-validate-pango

test-unattended: all
	@test/run_unattended_tests.sh

# This test requires to compile with DEBUG+=-DMTX_DEBUG=0 or higher.
test-validate-pango: all
	@test/validate_pango_markup.sh

### build distribution package
package: clean
	@echo "TODO $@"; false

### run C preprocessor
%.i: %.c
	$(CC) -E $< -o $@ $(CFLAGS) $(LIBS)

