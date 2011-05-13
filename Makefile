SUBDIRS = src/framework \
		  src/mlt++ \
		  src/melt \
		  src/modules \
		  src/swig \
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
	install -c -m 644 *.pc "$(DESTDIR)$(libdir)/pkgconfig"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done
	cp -R presets "$(DESTDIR)$(prefix)/share/mlt"

uninstall:
	rm -f "$(DESTDIR)$(bindir)"/mlt-config
	rm -f "$(DESTDIR)$(libdir)"/pkgconfig/mlt-framework.pc
	rm -f "$(DESTDIR)$(libdir)"/pkgconfig/mlt++.pc
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done
	rm -rf "$(DESTDIR)$(prefix)/include/mlt"
	rm -rf "$(DESTDIR)$(prefix)/share/mlt"

dist:
	git archive --format=tar --prefix=mlt-$(version)/ v$(version) | gzip >mlt-$(version).tar.gz
