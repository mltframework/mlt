
FRAMEWORK_OBJS = mlt_frame.o \
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
				 mlt_repository.o

OBJS = $(FRAMEWORK_OBJS)

SRCS := $(OBJS:.o=.c)

CFLAGS = -g -Wall -D_FILE_OFFSET_BITS=64 -pthread 

LDFLAGS = -lm -ldl -lpthread

all:	libmlt.so

libmlt.so: $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(FRAMEWORK_OBJS) libmlt.so

ifneq ($(wildcard .depend),)
include .depend
endif
