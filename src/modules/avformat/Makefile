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

ifdef MMX_FLAGS
	CFLAGS+=-DSWSCALE
	LDFLAGS+=-lswscale$(AVFORMAT_SUFFIX)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		if [ $(LOCAL_FFMPEG) ] ; then \
			$(MAKE) -C ffmpeg all ; \
			cp -f ffmpeg/libavcodec/libavcodec$(AVFORMAT_SUFFIX)$(LIBSUF) ../../framework/libavcodec$(AVFORMAT_SUFFIX)$(LIBSUF) ; \
			cp -f ffmpeg/libavformat/libavformat$(AVFORMAT_SUFFIX)$(LIBSUF) ../../framework/libavformat$(AVFORMAT_SUFFIX)$(LIBSUF) ; \
			cp -f ffmpeg/libavutil/libavutil$(AVFORMAT_SUFFIX)$(LIBSUF) ../../framework/libavutil$(AVFORMAT_SUFFIX)$(LIBSUF) ; \
		fi
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dep ; fi
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg distclean ; fi
		rm -f .depend

clean:	
		#if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg clean ; fi
		rm -f ../../framework/libavcodec$(AVFORMAT_SUFFIX)$(LIBSUF) ../../framework/libavformat$(AVFORMAT_SUFFIX)$(LIBSUF) ../../framework/libavutil$(AVFORMAT_SUFFIX)$(LIBSUF) $(OBJS) $(TARGET) 

install: all
		install -m 755 $(TARGET) "$(DESTDIR)$(prefix)/share/mlt/modules"
		if [ $(LOCAL_FFMPEG) ] ; then \
			install -m 755 ../../framework/libavcodec$(AVFORMAT_SUFFIX)$(LIBSUF) "$(DESTDIR)$(libdir)" ; \
			install -m 755 ../../framework/libavformat$(AVFORMAT_SUFFIX)$(LIBSUF) "$(DESTDIR)$(libdir)" ; \
			install -m 755 ../../framework/libavutil$(AVFORMAT_SUFFIX)$(LIBSUF) "$(DESTDIR)$(libdir)" ; \
		fi

ifneq ($(wildcard .depend),)
include .depend
endif
