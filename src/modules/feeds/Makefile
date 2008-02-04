include ../../../config.mak

all:	

depend:

distclean:

clean:

install:	all
	install -d $(DESTDIR)$(prefix)/share/mlt/feeds/PAL
	install -d $(DESTDIR)$(prefix)/share/mlt/feeds/NTSC
	install -m 644 PAL/*.* $(DESTDIR)$(prefix)/share/mlt/feeds/PAL
	install -m 644 NTSC/*.* $(DESTDIR)$(prefix)/share/mlt/feeds/NTSC
