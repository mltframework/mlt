
AR = ar

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

CFLAGS=-Wall -g -D_FILE_OFFSET_BITS=64 -pthread

LDFLAGS=-ldv -lpthread

all: libvalerie.a

libvalerie.a: $(OBJS)
	$(AR) rvu $@ $(OBJS)
	ranlib $@

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) libvalerie.a 

ifneq ($(wildcard .depend),)
include .depend
endif
