include config.mak

SUBDIRS = src/framework \
		  src/modules \
		  src/inigo \
		  src/valerie \
		  src/miracle \
		  src/humperdink \
		  src/albino

all clean dist-clean depend:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done

install:
	install -d "$(prefix)/bin"
	install -d "$(prefix)/include"
	install -d "$(prefix)/lib"
	install -d "$(prefix)/share/mlt/modules"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done; \
	/sbin/ldconfig || true

