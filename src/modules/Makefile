include ../../config.mak
include make.inc

all clean depend:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir ] ; \
		then $(MAKE) -C $$subdir $@ || exit 1; \
		fi \
	done

distclean:
	rm -f consumers.dat filters.dat producers.dat transitions.dat make.inc; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir ] ; \
		then $(MAKE) -C $$subdir $@ || exit 1; \
		fi \
	done

install:
	install -m 644 producers.dat filters.dat transitions.dat consumers.dat "$(DESTDIR)$(prefix)/share/mlt/modules"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir ] ; \
		then $(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
		fi \
	done

