AWK = awk
CPP = cpp
GLIB_COMPILE_RESOURCES ?= glib-compile-resources

%.md: %.md.in make_md.sh Makefile ../src/mtxversion.h
	env SUB_CPPFLAGS="$(SUB_CPPFLAGS)" ./make_md.sh $< $@

RES_SRC ::= $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies all.gresource.xml)

all: all.gresource

# Generate ../src/mtxresdata.c
all.gresource: all.gresource.xml $(RES_SRC) Makefile ../Makefile
	@echo 'resources: $(RES_SRC)'
	$(GLIB_COMPILE_RESOURCES) --target ../src/mtxresdata.c --internal --generate-source $<
	touch $@

mtx1.css: ../src/mtxcolor.h ../src/mtxstylepango.h make_css1.awk Makefile
	$(AWK) -f make_css1.awk OUTF=$@ ../src/mtxcolor.h ../src/mtxstylepango.h

mtx2.css: github-markdown.css make_css2.sh Makefile
	./make_css2.sh $< $@

clean:
	$(RM) -v all.gresource usage.md mtx1.css mtx2.css gmon.out

.PHONY: all clean
