include ../../../config.mak
LDFLAGS=

all:	luma create_lumas
	@./create_lumas 

luma:	luma.c
# When cross-compiling, use the host OS compiler to build the luma
# binary because the files are generated at build time.
# Strips the CROSS prefix from the C compiler variable.
ifdef CROSS
	$(subst $(CROSS),,$(CC)) -o $@ luma.c
endif

create_lumas:

depend:

distclean:
	rm -rf PAL NTSC luma 

clean:
	rm -f luma 

install:	all
	install -d $(DESTDIR)$(mltdatadir)/lumas/PAL
	install -d $(DESTDIR)$(mltdatadir)/lumas/NTSC
	install -m 644 PAL/* $(DESTDIR)$(mltdatadir)/lumas/PAL
	install -m 644 NTSC/* $(DESTDIR)$(mltdatadir)/lumas/NTSC
