SUBDIRS = java perl python ruby tcl

all clean install:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -f $$subdir/Makefile -a -f .$$subdir ] ; \
		then $(MAKE) -C $$subdir $@ || exit 1; \
		fi \
	done

