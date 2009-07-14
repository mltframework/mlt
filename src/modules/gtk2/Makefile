CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak
include config.mak

TARGET = ../libmltgtk2$(LIBSUF)

OBJS = factory.o

ifdef USE_GTK2
OBJS += consumer_gtk2.o
CFLAGS += `pkg-config gtk+-2.0 --cflags`
LDFLAGS += `pkg-config gtk+-2.0 --libs`
endif

ifdef USE_PIXBUF
OBJS += producer_pixbuf.o pixops.o filter_rescale.o
CFLAGS += `pkg-config gdk-pixbuf-2.0 --cflags`
LDFLAGS += `pkg-config gdk-pixbuf-2.0 --libs`
endif

ifdef MMX_FLAGS
ifndef ARCH_X86_64
ASM_OBJS = have_mmx.o scale_line_22_yuv_mmx.o
endif
endif

ifdef USE_PANGO
OBJS += producer_pango.o
CFLAGS += `pkg-config pangoft2 --cflags`
LDFLAGS += `pkg-config pangoft2 --libs`
ifeq ($(targetos),FreeBSD)
LDFLAGS += -liconv
endif
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

have_mmx.o:
	$(CC) -o $@ -c have_mmx.S

scale_line_22_yuv_mmx.o: scale_line_22_yuv_mmx.S
	$(CC) -o $@ -c scale_line_22_yuv_mmx.S

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
