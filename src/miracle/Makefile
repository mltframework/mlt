include ../../config.mak

TARGET = miracle

APP_OBJS = miracle.o

LIB_OBJS = miracle_log.o \
	   miracle_server.o \
	   miracle_connection.o \
	   miracle_local.o \
	   miracle_unit.o \
	   miracle_commands.o \
	   miracle_unit_commands.o

OBJS = $(APP_OBJS) $(LIB_OBJS)

CFLAGS = -O3 -I .. -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -rdynamic

LDFLAGS = -L ../valerie -lvalerie -L ../framework -lmlt

ifeq ($(MLT_GPROF),true)
CFLAGS+=-p
LDFLAGS+=-p
endif

SRCS := $(OBJS:.o=.c)

all: 		$(TARGET)

$(TARGET): 	$(APP_OBJS) libmiracle.so
			$(CC) -o $@ $(APP_OBJS) -L. -lmiracle $(LDFLAGS)

libmiracle.so:	$(LIB_OBJS)
			$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS)

depend:		$(SRCS)
			$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
			rm -f .depend

clean:	
			rm -f $(OBJS) $(TARGET) libmiracle.so

install:	all
	install -d "$(bindir)"
	install -c -s -m 755 $(TARGET) "$(bindir)"
	install -m 755 libmiracle.so $(prefix)/lib/libmiracle.so
	mkdir -p "$(prefix)/include/mlt/miracle"
	install -m 644 \
			miracle_local.h \
            "$(prefix)/include/mlt/miracle"

ifneq ($(wildcard .depend),)
include .depend
endif
