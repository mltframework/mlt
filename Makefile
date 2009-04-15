SUBDIRS = src/framework \
		  src/inigo \
		  src/valerie \
		  src/miracle \
		  src/humperdink \
		  src/albino \
		  src/modules \
		  profiles

all clean:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -s -C $$subdir depend || exit 1; \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done

distclean:
	rm mlt-config packages.dat; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done; \
	rm config.mak;

dist-clean: distclean

include config.mak

install:
	install -d "$(DESTDIR)$(prefix)/bin"
	install -d "$(DESTDIR)$(prefix)/include"
	install -d "$(DESTDIR)$(libdir)"
	install -d "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(libdir)/pkgconfig"
	install -d "$(DESTDIR)$(prefix)/share/mlt"
	install -c -m 755 mlt-config "$(DESTDIR)$(bindir)"
	install -c -m 644 *.pc "$(DESTDIR)$(libdir)/pkgconfig"
	install -m 644 packages.dat "$(DESTDIR)$(prefix)/share/mlt/"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done; \
	if test -z "$(DESTDIR)"; then \
	  /sbin/ldconfig 2> /dev/null || true; \
	fi

uninstall:
	rm -f "$(DESTDIR)$(bindir)"/mlt-config
	rm -f "$(DESTDIR)$(libdir)/pkgconfig/mlt-*.pc"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done
	rm -rf "$(DESTDIR)$(prefix)/include/mlt"
	rm -rf "$(DESTDIR)$(prefix)/share/mlt"

dist:
	[ -d "mlt-$(version)" ] && rm -rf "mlt-$(version)" || echo
	svn export http://mlt.svn.sourceforge.net/svnroot/mlt/trunk/mlt "mlt-$(version)"
	tar -cvzf "mlt-$(version).tar.gz" "mlt-$(version)"
