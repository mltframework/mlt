include ../../../config.mak

all:	

depend:

distclean:

clean:

install:	all
	install -d "$(DESTDIR)$(mltdatadir)/feeds/PAL"
	install -d "$(DESTDIR)$(mltdatadir)/feeds/NTSC"
	install -m 644 PAL/*.* "$(DESTDIR)$(mltdatadir)/feeds/PAL"
	install -m 644 NTSC/*.* "$(DESTDIR)$(mltdatadir)/feeds/NTSC"
