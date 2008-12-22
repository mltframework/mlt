include ../../../config.mak
include config.mak

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

ifdef CODECS
OBJS += producer_avformat.o \
	    consumer_avformat.o
CFLAGS += -DCODECS
endif

CFLAGS+=-I../..

LDFLAGS+=-L../../framework

LDFLAGS+=-lavformat$(AVFORMAT_SUFFIX) -lavcodec$(AVFORMAT_SUFFIX) -lavutil$(AVFORMAT_SUFFIX) -lavdevice$(AVFORMAT_SUFFIX) $(EXTRA_LIBS) -lmlt

ifdef SWSCALE
	CFLAGS+=-DSWSCALE
	LDFLAGS+=-lswscale$(AVFORMAT_SUFFIX)
endif

ifdef LOCAL_FFMPEG
	LOCAL_FFMPEG_OBJS = ffmpeg/libavformat/libavformat$(AVFORMAT_SUFFIX) \
	                    ffmpeg/libavcodec/libavcodec$(AVFORMAT_SUFFIX) \
	                    ffmpeg/libavutil/libavutil$(AVFORMAT_SUFFIX) \
	                    ffmpeg/libavutil/libavdevice$(AVFORMAT_SUFFIX)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(LOCAL_FFMPEG_OBJS):
	if [ $(LOCAL_FFMPEG) ] ; then \
		$(MAKE) -C ffmpeg ffmpeg ; \
	fi

$(TARGET): $(OBJS) $(LOCAL_FFMPEG_OBJS)
	$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
	if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dep ; fi
	$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
	if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg distclean ; fi
	rm -f .depend

clean:	
	#if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg clean ; fi
	rm -f $(OBJS) ../libmltffmpeg$(LIBSUF) ../libmltavformat$(LIBSUF)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(prefix)/share/mlt/avformat"
	install -m 644 producer_avformat.yml "$(DESTDIR)$(prefix)/share/mlt/avformat"

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltavformat$(LIBSUF)"
	rm "$(DESTDIR)$(libdir)/mlt/libmltffmpeg$(LIBSUF)"
	rm -rf "$(DESTDIR)$(prefix)/share/mlt/avformat" 

ifneq ($(wildcard .depend),)
include .depend
endif
