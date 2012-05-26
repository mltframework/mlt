CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak
include config.mak

TARGET = ../libmltswfdec$(LIBSUF)

OBJS = producer_swfdec.o

ifeq ($(targetos), MinGW)
LDFLAGS += -Wl,enable-auto-import -lz
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(datadir)/mlt/swfdec"
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/swfdec"

ifneq ($(wildcard .depend),)
include .depend
endif
