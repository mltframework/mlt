CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

all:	luma create_lumas
	@./create_lumas 

luma:	luma.o
# When cross-compiling, use the host OS compiler to build the luma
# binary because the files are generated at build time.
# Strips the CROSS prefix from the C compiler variable.
ifdef CROSS
	$(subst $(CROSS),,$(CC)) -o $@ luma.o $(LDFLAGS)
else
	$(CC) -o $@ luma.o $(LDFLAGS)
endif

create_lumas:

depend: luma.c
	$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:
	rm -rf luma 9_16 16_9 NTSC PAL square

clean:
	rm -f luma luma.o .executed

install:	all
	install -d "$(DESTDIR)$(mltdatadir)/lumas/9_16"
	install -d "$(DESTDIR)$(mltdatadir)/lumas/16_9"
	install -d "$(DESTDIR)$(mltdatadir)/lumas/NTSC"
	install -d "$(DESTDIR)$(mltdatadir)/lumas/PAL"
	install -d "$(DESTDIR)$(mltdatadir)/lumas/square"
	install -m 644 9_16/* "$(DESTDIR)$(mltdatadir)/lumas/9_16"
	install -m 644 16_9/* "$(DESTDIR)$(mltdatadir)/lumas/16_9"
	install -m 644 NTSC/* "$(DESTDIR)$(mltdatadir)/lumas/NTSC"
	install -m 644 PAL/*  "$(DESTDIR)$(mltdatadir)/lumas/PAL"
	install -m 644 square/* "$(DESTDIR)$(mltdatadir)/lumas/square"
