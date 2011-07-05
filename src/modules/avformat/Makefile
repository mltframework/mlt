CFLAGS += -I../..
LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak
include config.mak

LDFLAGS += -lavformat$(AVFORMAT_SUFFIX)
LDFLAGS += -lavcodec$(AVFORMAT_SUFFIX)
LDFLAGS += -lavutil$(AVFORMAT_SUFFIX)
LDFLAGS += $(EXTRA_LIBS)

ifndef CODECS
TARGET = ../libmltffmpeg$(LIBSUF)
else
TARGET = ../libmltavformat$(LIBSUF)
endif

OBJS = factory.o

ifdef FILTERS
OBJS += filter_avcolour_space.o \
	    filter_avresample.o \
	    filter_avdeinterlace.o
ifdef SWSCALE
OBJS += filter_swscale.o
endif
CFLAGS += -DFILTERS
endif

ifdef VDPAU
CFLAGS += -DVDPAU `pkg-config --cflags x11`
LDFLAGS += $(LIBDL) `pkg-config --libs x11`
endif

ifdef CODECS
OBJS += producer_avformat.o \
	    consumer_avformat.o
CFLAGS += -DCODECS
endif

ifdef SWSCALE
CFLAGS += -DSWSCALE
LDFLAGS += -lswscale$(AVFORMAT_SUFFIX)
endif

ifdef DEVICES
CFLAGS += -DAVDEVICE
LDFLAGS += -lavdevice$(AVFORMAT_SUFFIX)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
	$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
	rm -f .depend

clean:	
	rm -f $(OBJS) ../libmltffmpeg$(LIBSUF) ../libmltavformat$(LIBSUF)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(datadir)/mlt/avformat"
	install -m 644 producer_avformat.yml "$(DESTDIR)$(datadir)/mlt/avformat"
	install -m 644 consumer_avformat.yml "$(DESTDIR)$(datadir)/mlt/avformat"

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltavformat$(LIBSUF)" 2> /dev/null || true
	rm "$(DESTDIR)$(libdir)/mlt/libmltffmpeg$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(datadir)/mlt/avformat"

ifneq ($(wildcard .depend),)
include .depend
endif
