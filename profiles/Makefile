include ../config.mak

all:

depend:

distclean:

clean:

install:	all uninstall
	install -d "$(DESTDIR)$(mltdatadir)/profiles"
	install -m 644 * "$(DESTDIR)$(mltdatadir)/profiles"
	rm -f "$(DESTDIR)$(mltdatadir)/profiles/"*~
	rm -f "$(DESTDIR)$(mltdatadir)/profiles/Makefile"

uninstall:
	rm -rf "$(DESTDIR)$(mltdatadir)/profiles"
