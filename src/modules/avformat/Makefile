include ../../../config.mak

TARGET = ../libmltavformat.so

OBJS = factory.o \
	   producer_avformat.o \
	   consumer_avformat.o

CFLAGS = -O4 -pipe -ffast-math -fomit-frame-pointer -I../../ -Wall -g -D_FILE_OFFSET_BITS=64

LDFLAGS = -lavformat -lavcodec

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
