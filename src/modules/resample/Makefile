
TARGET = ../libmltresample.so

OBJS = factory.o \
	   filter_resample.o 

CFLAGS = -O3 -I../../ -Wall -g

LDFLAGS= -lsamplerate

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
