include ../../config.mak

TARGET = miracle

ifneq ($(targetos), Darwin)
LIBNAME = libmiracle$(LIBSUF)
LIBTARGET = $(LIBNAME).$(version)
SHFLAGS += -Wl,-soname,$(LIBTARGET)
else
LIBNAME = libmiracle$(LIBSUF)
LIBTARGET = libmiracle.$(version)$(LIBSUF)
SHFLAGS += -install_name $(libdir)/$(LIBTARGET)
endif

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

$(TARGET): 	$(APP_OBJS) $(LIBTARGET)
			$(CC) -o $@ $(APP_OBJS) -L. -lmiracle $(LDFLAGS)

$(LIBTARGET):	$(LIB_OBJS)
			$(CC) $(SHFLAGS) -o $@ $(LIB_OBJS) $(LDFLAGS)
			ln -sf $(LIBTARGET) $(LIBNAME)

depend:		$(SRCS)
			$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
			rm -f .depend

clean:	
			rm -f $(OBJS) $(TARGET) $(LIBNAME) $(LIBTARGET)

install:	all
	install -d "$(DESTDIR)$(bindir)"
	install -c -s -m 755 $(TARGET) "$(DESTDIR)$(bindir)"
	install -m 755 $(LIBTARGET) $(DESTDIR)$(libdir)
	ln -sf $(LIBTARGET) $(DESTDIR)$(libdir)/$(LIBNAME)
	mkdir -p "$(DESTDIR)$(prefix)/include/mlt/miracle"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/miracle"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(LIBTARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(LIBNAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt/miracle"

ifneq ($(wildcard .depend),)
include .depend
endif
