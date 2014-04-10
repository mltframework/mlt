include ../../../config.mak
include config.mak

CFLAGS := -I../.. $(CFLAGS)

LDFLAGS := -L../../framework -lmlt -lpthread -lm $(LDFLAGS)

TARGET = ../libmltgtk2$(LIBSUF)

OBJS = factory.o

ifdef USE_GTK2
OBJS += consumer_gtk2.o
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags gtk+-2.0)
LDFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --libs gtk+-2.0)
endif

ifdef USE_PIXBUF
OBJS += producer_pixbuf.o pixops.o filter_rescale.o
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags gdk-pixbuf-2.0)
LDFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --libs gdk-pixbuf-2.0)
endif

ifdef USE_EXIF
CFLAGS += $(EXIFCXXFLAGS) 
LDFLAGS += $(EXIFLIBS)
endif

ifdef MMX_FLAGS
ifndef ARCH_X86_64
ASM_OBJS = have_mmx.o scale_line_22_yuv_mmx.o
endif
endif

ifdef USE_PANGO
OBJS += producer_pango.o
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags pangoft2)
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags-only-I freetype2 | awk '{for (i=1; i<=NF; i++) $$i=sprintf("%s/freetype", $$i); print}')
LDFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --libs pangoft2)
ifeq ($(targetos),Darwin)
LDFLAGS += -liconv
endif
ifeq ($(targetos),FreeBSD)
LDFLAGS += -liconv
endif
ifeq ($(targetos), MinGW)
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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/gtk2"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/gtk2"

ifneq ($(wildcard .depend),)
include .depend
endif
