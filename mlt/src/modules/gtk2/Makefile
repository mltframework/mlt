
TARGET = ../libmltgtk2.so

OBJS = factory.o \
	   producer_pixbuf.o 

CFLAGS = `pkg-config gdk-pixbuf-2.0 --cflags` -I../../ -Wall -g -D_FILE_OFFSET_BITS=64 -pthread

LDFLAGS = `pkg-config gdk-pixbuf-2.0 --libs`

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
