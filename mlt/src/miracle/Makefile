TARGET = miracle

OBJS = miracle.o \
	   miracle_log.o \
	   miracle_server.o \
	   miracle_connection.o \
	   miracle_local.o \
	   miracle_unit.o \
	   miracle_commands.o \
	   miracle_unit_commands.o

CFLAGS = -I .. -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -rdynamic

LDFLAGS = -L ../valerie -lvalerie -L ../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

ifneq ($(wildcard .depend),)
include .depend
endif
