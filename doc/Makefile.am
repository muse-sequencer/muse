EXTRA_DIST = midieditfunctions.png muse.sgm

CLEANFILES = $(wildcard *.html) $(wildcard *.fot) doxygen-stamp

all-local: @DOCBOOKTARGETS@

manual: book1.html

book1.html: muse.sgm
	$(JADE) -t sgml -d @DOCBOOKSTYLE@/html/docbook.dsl muse.sgm

distclean-local:
	rm -rf html/
