include ../../../config.mak

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
	install -d $(prefix)/share/mlt/modules/lumas/PAL
	install -d $(prefix)/share/mlt/modules/lumas/NTSC
	install -m 644 PAL/* $(prefix)/share/mlt/modules/lumas/PAL
	install -m 644 NTSC/* $(prefix)/share/mlt/modules/lumas/NTSC
