SUBDIRS = core gtk2 dv sdl # bluefish mcmpeg

all clean depend install:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		[ ! -f disable-$$subdir ] && $(MAKE) -C $$subdir $@; \
	done

dist-clean:
	rm -f consumers.dat filters.dat producers.dat transitions.dat; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		[ ! -f disable-$$subdir ] && $(MAKE) -C $$subdir $@; \
	done

