include $(top_srcdir)/misc/Makefile.quiet
include $(top_srcdir)/misc/Makefile.common

doc_DATA = faq.txt
html_DATA = faq.html

EXTRA_DIST = README faq.docbook faq.xsl $(doc_DATA) $(html_DATA)

DISTCLEANFILES = $(doc_DATA) $(html_DATA)

docs: $(hackersguide_DATA)

clean-docs:
	rm -f $(hackersguide_DATA)

dist-hook:
	@if test x"$(distcheck_lax)" = x ; then \
	  $(MAKE) fail_if_missing=yes docs ; \
	else \
	  $(MAKE) docs ; \
	fi

distclean-local: clean-docs

if HAVE_XMLTO
faq.html: faq.docbook faq.xsl
	xmlto -m faq.xsl xhtml-nochunks $<
faq.txt: faq.docbook faq.xsl
	xmlto -m faq.xsl txt $<
else
faq.html faq.txt: faq.docbook
	if test x"$(fail_if_missing)" = x"yes"; then \
	  echo "Please install xmlto."; \
	  exit 1; \
	fi
	if test x"$(fail_if_missing)" != x"yes"; then \
	  touch $@; \
	  sleep 1; \
	  touch $^; \
	fi
endif
