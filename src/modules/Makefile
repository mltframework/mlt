include make.inc

all clean depend install:
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

