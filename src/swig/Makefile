include ../../config.mak
include config.mak

all clean:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		if [ -x $$subdir/build -a ! -f .$$subdir -o $@ = clean ] ; \
		then echo -n Building $$subdir... ; \
			cd $$subdir && output=$$(CXXFLAGS="$(CXXFLAGS)" ./build $@ 2>&1) ; \
			if [ $$? -eq 0 ] ; \
			then echo OK && touch ../.$$subdir ; \
			else echo $$output && exit 1 ; \
			fi ; \
			cd .. ; \
			if [ -f $$subdir/Makefile -a -f .$$subdir ] ; \
			then $(MAKE) -C $$subdir $@ || exit 1 ; \
			fi ; \
			if [ $@ = clean ] ; \
			then rm -f .$$subdir ; \
			fi ; \
		fi \
	done

distclean: clean

depend:

install:

uninstall:
