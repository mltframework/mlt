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

INCS = miracle_server.h \
	   miracle_local.h \
	   miracle_log.h

OBJS = $(APP_OBJS) $(LIB_OBJS)

CFLAGS += -I.. $(RDYNAMIC)

LDFLAGS += -L../valerie -lvalerie -L../framework -lmlt 

SRCS := $(OBJS:.o=.c)

all: 		$(TARGET)

$(TARGET): 	$(APP_OBJS) libmiracle$(LIBSUF)
			$(CC) -o $@ $(APP_OBJS) -L. -lmiracle $(LDFLAGS)

libmiracle$(LIBSUF):	$(LIB_OBJS)
			$(CC) $(SHFLAGS) -o $@ $(LIB_OBJS) $(LDFLAGS)

depend:		$(SRCS)
			$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
			rm -f .depend

clean:	
			rm -f $(OBJS) $(TARGET) libmiracle$(LIBSUF)

install:	all
	install -d "$(DESTDIR)$(bindir)"
	install -c -s -m 755 $(TARGET) "$(DESTDIR)$(bindir)"
	install -m 755 libmiracle$(LIBSUF) $(DESTDIR)$(libdir)/libmiracle$(LIBSUF)
	mkdir -p "$(DESTDIR)$(prefix)/include/mlt/miracle"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/miracle"

ifneq ($(wildcard .depend),)
include .depend
endif
