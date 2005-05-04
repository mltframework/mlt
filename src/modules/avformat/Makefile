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

ifdef LOCAL_FFMPEG 
	LDFLAGS+=-lavformat.mlt -lavcodec.mlt $(EXTRA_LIBS) -lmlt
else
	LDFLAGS+=-lavformat -lavcodec $(EXTRA_LIBS) -lmlt
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		if [ $(LOCAL_FFMPEG) ] ; then \
			$(MAKE) -C ffmpeg all ; \
			cp -f ffmpeg/libavcodec/libavcodec$(LIBSUF) ../../framework/libavcodec.mlt$(LIBSUF) ; \
			cp -f ffmpeg/libavformat/libavformat$(LIBSUF) ../../framework/libavformat.mlt$(LIBSUF) ; \
		fi
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dep ; fi
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dist-clean ; fi
		rm -f .depend

clean:	
		#if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg clean ; fi
		rm -f $(OBJS) $(TARGET) 

install: all
		install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"
		if [ $(LOCAL_FFMPEG) ] ; then \
			install -m 755 ../../framework/libavcodec.mlt$(LIBSUF) "$(prefix)/lib" ; \
			install -m 755 ../../framework/libavformat.mlt$(LIBSUF) "$(prefix)/lib" ; \
		fi

ifneq ($(wildcard .depend),)
include .depend
endif
