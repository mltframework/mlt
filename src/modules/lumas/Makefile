include ../../../config.mak
LDFLAGS=

all:	luma create_lumas
	@./create_lumas 

luma:	luma.c

create_lumas:

depend:

distclean:
	rm -rf PAL NTSC luma 

clean:
	rm -f luma 

install:	all
	install -d $(DESTDIR)$(datadir)/mlt/lumas/PAL
	install -d $(DESTDIR)$(datadir)/mlt/lumas/NTSC
	install -m 644 PAL/* $(DESTDIR)$(datadir)/mlt/lumas/PAL
	install -m 644 NTSC/* $(DESTDIR)$(datadir)/mlt/lumas/NTSC
