include ../../config.mak

include make.inc

all clean depend:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile ] ; \
		then [ ! -f disable-$$subdir ] && $(MAKE) -C $$subdir $@; \
		fi \
	done

dist-clean:
	rm -f consumers.dat filters.dat producers.dat transitions.dat make.inc; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile ] ; \
		then [ ! -f disable-$$subdir ] && $(MAKE) -C $$subdir $@; \
		fi \
	done

install:
	mkdir -p "$(prefix)/share/mlt/modules"
	install -m 644 producers.dat filters.dat transitions.dat consumers.dat "$(prefix)/share/mlt/modules"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile ] ; \
		then [ ! -f disable-$$subdir ] && $(MAKE) -C $$subdir $@; \
		fi \
	done; \
	/sbin/ldconfig

