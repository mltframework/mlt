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
	rm mlt-config packages.dat config.mak; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done


install:
	install -d "$(prefix)/bin"
	install -d "$(prefix)/include"
	install -d "$(prefix)/lib"
	install -d "$(prefix)/share/mlt/modules"
	install -c -m 755 mlt-config "$(bindir)"
	install -m 644 packages.dat "$(prefix)/share/mlt/"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done; \
	/sbin/ldconfig || true

