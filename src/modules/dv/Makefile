include ../../../config.mak

TARGET = ../libmltdv.so

OBJS = factory.o \
	   producer_libdv.o \
	   consumer_libdv.o

CFLAGS = -O3 -I../../ -Wall -g -D_FILE_OFFSET_BITS=64 -pthread

LDFLAGS=-ldv -lpthread

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
