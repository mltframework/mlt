include ../../../config.mak
LDFLAGS=

all:	luma create_lumas
	@./create_lumas 

luma:	luma.c

create_lumas:

depend:

dist-clean:
	rm -rf PAL NTSC luma 

clean:
	rm -f luma 

install:	all
	install -d $(DESTDIR)$(prefix)/share/mlt/modules/lumas/PAL
	install -d $(DESTDIR)$(prefix)/share/mlt/modules/lumas/NTSC
	install -m 644 PAL/* $(DESTDIR)$(prefix)/share/mlt/modules/lumas/PAL
	install -m 644 NTSC/* $(DESTDIR)$(prefix)/share/mlt/modules/lumas/NTSC
