include ../../../config.mak

TARGET = ../libmltffmpeg.so

OBJS = factory.o \
	   producer_ffmpeg.o \
	   filter_ffmpeg_dub.o \
	   consumer_ffmpeg.o

CFLAGS += -I../..

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
