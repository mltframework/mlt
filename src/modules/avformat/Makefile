include ../../../config.mak
include config.mak

TARGET = ../libmltavformat.so

OBJS = factory.o \
	   producer_avformat.o \
	   consumer_avformat.o \
	   filter_avdeinterlace.o \
	   filter_avresample.o

CFLAGS+=-I../..

LDFLAGS+=-lavformat -lavcodec $(EXTRA_LIBS)

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg all ; fi
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dep ; fi
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		if [ $(LOCAL_FFMPEG) ] ; then $(MAKE) -C ffmpeg dist-clean ; fi
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) 

install: all
		install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
