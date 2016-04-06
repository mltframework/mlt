CFLAGS += -I../..
LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak
include config.mak

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
	    filter_avdeinterlace.o \
	    filter_swscale.o
CFLAGS += -DFILTERS
endif

ifdef VDPAU
CFLAGS += -DVDPAU $(shell pkg-config pkg-config --cflags x11)
LDFLAGS += $(LIBDL) $(shell pkg-config pkg-config --libs x11)
endif

ifdef CODECS
OBJS += producer_avformat.o \
	    consumer_avformat.o
CFLAGS += -DCODECS
endif

ifdef DEVICES
CFLAGS += -DAVDEVICE
endif

ifdef AVFILTER
CFLAGS += -DAVFILTER
OBJS += filter_avfilter.o
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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/avformat"
	install -m 644 blacklist.txt "$(DESTDIR)$(mltdatadir)/avformat"
	install -m 644 producer_avformat.yml "$(DESTDIR)$(mltdatadir)/avformat"
	install -m 644 consumer_avformat.yml "$(DESTDIR)$(mltdatadir)/avformat"

uninstall:
	rm "$(DESTDIR)$(moduledir)/libmltavformat$(LIBSUF)" 2> /dev/null || true
	rm "$(DESTDIR)$(moduledir)/libmltffmpeg$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(mltdatadir)/avformat"

ifneq ($(wildcard .depend),)
include .depend
endif
