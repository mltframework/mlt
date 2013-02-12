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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/swfdec"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/swfdec"

ifneq ($(wildcard .depend),)
include .depend
endif
