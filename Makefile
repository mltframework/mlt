include config.mak

SUBDIRS = src/framework \
		  src/modules \
		  src/inigo \
		  src/valerie \
		  src/miracle \
		  src/humperdink \
		  src/albino

all clean dist-clean depend install:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done

