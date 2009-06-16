include ../../../config.mak

all:	

depend:

distclean:

clean:

install:	all
	install -d "$(DESTDIR)$(datadir)/mlt/feeds/PAL"
	install -d "$(DESTDIR)$(datadir)/mlt/feeds/NTSC"
	install -m 644 PAL/*.* "$(DESTDIR)$(datadir)/mlt/feeds/PAL"
	install -m 644 NTSC/*.* "$(DESTDIR)$(datadir)/mlt/feeds/NTSC"
