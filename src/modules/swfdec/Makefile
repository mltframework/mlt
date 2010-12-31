CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltswfdec$(LIBSUF)

OBJS = producer_swfdec.o

CFLAGS += `pkg-config --cflags swfdec-0.8 2>/dev/null`
CFLAGS += `pkg-config --cflags swfdec-0.9 2>/dev/null`

LDFLAGS += `pkg-config --libs swfdec-0.8 2>/dev/null`
LDFLAGS += `pkg-config --libs swfdec-0.9 2>/dev/null`

ifeq ($(targetos), MinGW)
LDFLAGS += -enable-auto-import -lz
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

ifneq ($(wildcard .depend),)
include .depend
endif
