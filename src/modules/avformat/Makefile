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

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		if [ $(LOCAL_FFMPEG) ] ; then \
			$(MAKE) -C ffmpeg lib ; \
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
		rm -f $(OBJS) $(TARGET)

install: all
		install -m 755 $(TARGET) "$(DESTDIR)$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
