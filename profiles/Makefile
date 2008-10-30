include ../config.mak

all:

depend:

distclean:

clean:

install:	all uninstall
	install -d "$(DESTDIR)$(prefix)/share/mlt/profiles"
	install -m 644 * "$(DESTDIR)$(prefix)/share/mlt/profiles"
	rm -f "$(DESTDIR)$(prefix)/share/mlt/profiles/"*~
	rm -f "$(DESTDIR)$(prefix)/share/mlt/profiles/Makefile"

uninstall:
	rm -rf "$(DESTDIR)$(prefix)/share/mlt/profiles"
