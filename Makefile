SUBDIRS = src/framework src/valerie src/modules # src/miracle src/humperdink

all clean dist-clean depend install:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@; \
	done

