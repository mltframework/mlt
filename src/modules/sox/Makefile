CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

include config.mak

TARGET = ../libmltsox$(LIBSUF)

OBJS = factory.o \
	   filter_sox.o 

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
	install -d "$(DESTDIR)$(datadir)/mlt/sox"
	install -m 644 filter_sox.yml "$(DESTDIR)$(datadir)/mlt/sox"

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltsox$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(datadir)/mlt/sox"

ifneq ($(wildcard .depend),)
include .depend
endif
