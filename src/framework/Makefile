
TARGET = libmlt.so

OBJS = mlt_frame.o \
	   mlt_deque.o \
	   mlt_property.o \
	   mlt_properties.o \
	   mlt_service.o \
	   mlt_producer.o \
	   mlt_multitrack.o \
	   mlt_playlist.o \
	   mlt_consumer.o \
	   mlt_filter.o \
	   mlt_transition.o \
	   mlt_field.o \
	   mlt_tractor.o \
	   mlt_factory.o \
	   mlt_repository.o \
	   mlt_pool.o

SRCS := $(OBJS:.o=.c)

CFLAGS = -g -O3 -Wall -D_FILE_OFFSET_BITS=64 -pthread 

LDFLAGS = -lm -ldl -lpthread

all:	$(TARGET)

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
