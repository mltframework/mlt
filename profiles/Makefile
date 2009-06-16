include ../config.mak

all:

depend:

distclean:

clean:

install:	all uninstall
	install -d "$(DESTDIR)$(datadir)/mlt/profiles"
	install -m 644 * "$(DESTDIR)$(datadir)/mlt/profiles"
	rm -f "$(DESTDIR)$(datadir)/mlt/profiles/"*~
	rm -f "$(DESTDIR)$(datadir)/mlt/profiles/Makefile"

uninstall:
	rm -rf "$(DESTDIR)$(datadir)/mlt/profiles"
