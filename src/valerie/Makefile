include ../../config.mak

TARGET = libvalerie.so

OBJS = valerie.o \
	   valerie_notifier.o \
	   valerie_parser.o \
	   valerie_response.o \
	   valerie_status.o \
	   valerie_tokeniser.o \
	   valerie_util.o \
	   valerie_remote.o \
	   valerie_socket.o

SRCS := $(OBJS:.o=.c)

LDFLAGS=-ldv -lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: 	all
	install -m 755 $(TARGET) $(prefix)/lib/libvalerie.so
	mkdir -p "$(prefix)/include/mlt/valerie"
	install -m 644 \
			valerie.h \
			valerie_notifier.h \
			valerie_parser.h \
			valerie_remote.h \
			valerie_response.h \
			valerie_socket.h \
			valerie_status.h \
			valerie_tokeniser.h \
			valerie_util.h \
            "$(prefix)/include/mlt/valerie"

ifneq ($(wildcard .depend),)
include .depend
endif
