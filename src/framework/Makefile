include ../../config.mak

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

CFLAGS += -pthread -DPREFIX="\"$(prefix)\""

LDFLAGS += -lm $(LIBDL) -lpthread

all:	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install:
	install -m 755 $(TARGET) $(prefix)/lib/libmlt.so
	mkdir -p "$(prefix)/include/mlt/framework"
	install -m 644 mlt_consumer.h \
			mlt_factory.h \
			mlt_filter.h \
			mlt.h \
			mlt_multitrack.h \
			mlt_pool.h \
			mlt_properties.h \
			mlt_repository.h \
			mlt_tractor.h \
			mlt_types.h \
			mlt_deque.h \
			mlt_field.h \
			mlt_frame.h \
			mlt_playlist.h \
			mlt_producer.h \
			mlt_property.h \
			mlt_service.h  \
			mlt_transition.h \
            "$(prefix)/include/mlt/framework"

ifneq ($(wildcard .depend),)
include .depend
endif
