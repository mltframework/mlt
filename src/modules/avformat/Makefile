CFLAGS += -I../..
LDFLAGS += -L../../framework -lmlt -lpthread -lm

ifneq (, $(shell $(CC) --version | grep -is -e 'Free Software Foundation'))
	LDFLAGS += -latomic
endif

include ../../../config.mak
include config.mak

LDFLAGS += $(EXTRA_LIBS)

ifndef CODECS
TARGET = ../libmltffmpeg$(LIBSUF)
else
TARGET = ../libmltavformat$(LIBSUF)
endif

OBJS = common.o \
       factory.o

ifdef FILTERS
OBJS += filter_avcolour_space.o \
	    filter_avdeinterlace.o \
	    filter_swscale.o
CFLAGS += -DFILTERS
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

ifdef SWRESAMPLE
OBJS += filter_swresample.o
CFLAGS += -DSWRESAMPLE
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
	install -m 644 yuv_only.txt "$(DESTDIR)$(mltdatadir)/avformat"
	install -m 644 producer_avformat.yml "$(DESTDIR)$(mltdatadir)/avformat"
	install -m 644 consumer_avformat.yml "$(DESTDIR)$(mltdatadir)/avformat"

uninstall:
	rm -f "$(DESTDIR)$(moduledir)/libmltavformat$(LIBSUF)"
	rm -f "$(DESTDIR)$(moduledir)/libmltffmpeg$(LIBSUF)"
	rm -rf "$(DESTDIR)$(mltdatadir)/avformat"

ifneq ($(wildcard .depend),)
include .depend
endif
