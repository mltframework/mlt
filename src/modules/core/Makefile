
TARGET = ../libmltcore.so

OBJS = factory.o \
	   producer_ppm.o \
	   filter_deinterlace.o \
	   filter_greyscale.o \
	   filter_resize.o \
	   transition_composite.o

CFLAGS = -I../../ -Wall -g -D_FILE_OFFSET_BITS=64 -pthread

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
