CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak

TARGET = ../libmltjackrack$(LIBSUF)

OBJS = factory.o \
	   jack_rack.o \
	   lock_free_fifo.o \
	   plugin.o \
	   plugin_desc.o \
	   plugin_mgr.o \
	   plugin_settings.o \
	   process.o \
	   filter_jackrack.o \
	   filter_ladspa.o

CFLAGS += `pkg-config --cflags jack`
CFLAGS += `pkg-config --cflags libxml-2.0`
CFLAGS += `pkg-config --cflags glib-2.0`

LDFLAGS += $(LIBDL)
LDFLAGS += `pkg-config --libs jack`
LDFLAGS += `pkg-config --libs libxml-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

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
	install -d "$(DESTDIR)$(datadir)/mlt/jackrack"
	install -m 644 filter_jackrack.yml "$(DESTDIR)$(datadir)/mlt/jackrack"
	install -m 644 filter_ladspa.yml "$(DESTDIR)$(datadir)/mlt/jackrack"
	install -m 644 blacklist.txt "$(DESTDIR)$(datadir)/mlt/jackrack"

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltjackrack$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(datadir)/mlt/jackrack"

ifneq ($(wildcard .depend),)
include .depend
endif
