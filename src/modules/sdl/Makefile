
TARGET = ../libmltsdl.so

OBJS = factory.o \
	   consumer_sdl.o 

CFLAGS = -O3 -I../../ `sdl-config --cflags` -Wall -g -D_FILE_OFFSET_BITS=64 -pthread

LDFLAGS= `sdl-config --libs`

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

ifneq ($(wildcard .depend),)
include .depend
endif
