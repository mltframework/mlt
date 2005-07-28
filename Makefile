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
	install -d "$(prefix)/bin"
	install -d "$(prefix)/include"
	install -d "$(prefix)/lib"
	install -d "$(prefix)/lib/pkgconfig"
	install -d "$(prefix)/share/mlt/modules"
	install -c -m 755 mlt-config "$(bindir)"
	install -c -m 644 *.pc "$(prefix)/lib/pkgconfig"
	install -m 644 packages.dat "$(prefix)/share/mlt/"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done; \
	/sbin/ldconfig || true
