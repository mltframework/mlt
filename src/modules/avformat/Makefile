include ../../../config.mak
include config.mak

TARGET = ../libmltavformat$(LIBSUF)

OBJS = factory.o \
	   producer_avformat.o \
	   consumer_avformat.o \
	   filter_avcolour_space.o \
	   filter_avresample.o

ifdef MMX_FLAGS
	OBJS += filter_avdeinterlace.o
endif

CFLAGS+=-I../..

LDFLAGS+=-L../../framework

LDFLAGS+=-lavformat$(AVFORMAT_SUFFIX) -lavcodec$(AVFORMAT_SUFFIX) -lavutil$(AVFORMAT_SUFFIX) $(EXTRA_LIBS) -lmlt

ifdef SWSCALE
	CFLAGS+=-DSWSCALE
	LDFLAGS+=-lswscale$(AVFORMAT_SUFFIX)
endif

ifdef LOCAL_FFMPEG
	LOCAL_FFMPEG_OBJS = ffmpeg/libavformat/libavformat$(AVFORMAT_SUFFIX) \
	                    ffmpeg/libavcodec/libavcodec$(AVFORMAT_SUFFIX) \
	                    ffmpeg/libavutil/libavutil$(AVFORMAT_SUFFIX)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(LOCAL_FFMPEG_OBJS):
		if [ $(LOCAL_FFMPEG) ] ; then \
			$(MAKE) -C ffmpeg lib ; \
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
		rm -f $(OBJS) $(TARGET)

install: all
		install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
