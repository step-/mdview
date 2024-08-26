GLIB_COMPILE_RESOURCES ?= glib-compile-resources

RES_SRC ::= $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies all.gresource.xml)

all.gresource: all.gresource.xml $(RES_SRC) Makefile
	$(GLIB_COMPILE_RESOURCES) --generate-source $<
	touch $@

clean:
	$(RM) -v all.gresource

.PHONY: clean
