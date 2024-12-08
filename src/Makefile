SED = sed

SRC ::= \
	main.c \
	mtxviewer.c \
	mtxtextview.c \
	entity.c \
	md4c.c \
	mtxrender.c \
	mtx.c \
	mtxcmm.c \
	mtxdbg.c \
	mtxmarkup.c \
	mtxresdata.c

INCL ::= \
	main.decl.h \
	mtxcolor.h \
	mtxstylepango.h \
	mtxviewer.h mtxviewer.decl.h \
	mtxresources.h \
	mtxtextview.h mtxtextview.decl.h \
	mtxtextviewprivate.h \
	entity.h \
	md4c.h \
	mtxrender.h \
	mtx.h mtx.decl.h \
	mtxcmm.h mtxcmm.decl.h \
	mtxcmmprivate.h \
	mtxdbg.h \
	mtxmarkup.h \

DEPS    ::= $(SRC) $(INCL) Makefile ../Makefile

all: mdview

mdview: $(DEPS) mtxversion.h ../resources/all.gresource
	$(CC) $(ALL_FLAGS) -o mdview $(SRC) $(LIBS)

mtxversion.h: mtxversion.h.in ../Makefile
	@echo 'making $@ ...'
	@$(SED) -e 's|@PACKAGE_NAME@|$(PACKAGE_NAME)|' \
		-e 's|@PACKAGE_VERSION@|$(PACKAGE_VERSION)|' \
		-e 's|@PACKAGE_URL@|$(PACKAGE_URL)|' \
		-e 's|@GETTEXT_PACKAGE@|$(GETTEXT_PACKAGE)|' \
		-e 's|@LOCALEDIR@|$(LOCALEDIR)|' \
	$< > $@

clean:
	$(RM) -v mdview mtxversion.h

### extract function declarations
### e.g., make main.decl.h     reads main.c, overwrites main.decl.h
%.decl.h: %.c
	./make_decl.sh $< > $@

### run the C preprocessor
%.i: %.c
	$(CC) $(ALL_FLAGS) -E $< -o $@

.PHONY: all clean
