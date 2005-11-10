include config.mak

SUBDIRS = src/framework \
		  src/inigo \
		  src/valerie \
		  src/miracle \
		  src/humperdink \
		  src/albino \
		  src/modules

all clean:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -s -C $$subdir depend || exit 1; \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done

dist-clean:
	rm mlt-config packages.dat; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done; \
	rm config.mak;

install:
	install -d "$(DESTDIR)$(prefix)/bin"
	install -d "$(DESTDIR)$(prefix)/include"
	install -d "$(DESTDIR)$(prefix)/lib"
	install -d "$(DESTDIR)$(prefix)/lib/pkgconfig"
	install -d "$(DESTDIR)$(prefix)/share/mlt/modules"
	install -c -m 755 mlt-config "$(DESTDIR)$(bindir)"
	install -c -m 644 *.pc "$(DESTDIR)$(prefix)/lib/pkgconfig"
	install -m 644 packages.dat "$(DESTDIR)$(prefix)/share/mlt/"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done; \
	if test -z "$(DESTDIR)"; then \
	  /sbin/ldconfig || true; \
	fi
