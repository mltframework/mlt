include ../../config.mak
include make.inc

all clean depend:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir -a ! -f $$subdir/deprecated ] ; \
		then $(MAKE) -C $$subdir $@ || exit 1; \
		fi \
	done

distclean:
	rm -f consumers.dat filters.dat producers.dat transitions.dat
	echo > make.inc
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir -a ! -f $$subdir/deprecated ] ; \
		then $(MAKE) -C $$subdir $@ || exit 1; \
		fi \
	done

install:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a ! -f disable-$$subdir -a ! -f $$subdir/deprecated ] ; \
		then $(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
		fi \
	done

uninstall:
	rm -rf "$(DESTDIR)$(moduledir)"

