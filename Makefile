include config.mak

SUBDIRS = src/framework \
		  src/modules \
		  src/inigo \
		  src/valerie \
		  src/miracle \
		  src/humperdink \
		  src/albino

all clean depend:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done

dist-clean:
	rm mlt-config packages.dat; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
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
		$(MAKE) -C $$subdir $@; \
	done; \
	/sbin/ldconfig || true
