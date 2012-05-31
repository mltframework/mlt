CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak
include config.mak

TARGET = ../libmltjackrack$(LIBSUF)

GPL_OBJS = jack_rack.o \
	lock_free_fifo.o \
	plugin.o \
	plugin_desc.o \
	plugin_mgr.o \
	plugin_settings.o \
	process.o \
	filter_jackrack.o \
	filter_ladspa.o

OBJS = factory.o \
	   consumer_jack.o

CFLAGS += `pkg-config --cflags jack`
LDFLAGS += `pkg-config --libs jack`

ifdef GPL
OBJS += $(GPL_OBJS)
CFLAGS += -DGPL
CFLAGS += `pkg-config --cflags libxml-2.0`
CFLAGS += `pkg-config $(PKGCONFIG_PREFIX) --cflags glib-2.0`

LDFLAGS += $(LIBDL)
LDFLAGS += `pkg-config --libs libxml-2.0`
LDFLAGS += `pkg-config $(PKGCONFIG_PREFIX) --libs glib-2.0`
LDFLAGS += -lm

YML_FILES = *.yml
BLACKLIST = blacklist.txt
else
YML_FILES = consumer_jack.yml
BLACKLIST = dummy
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
	install -d "$(DESTDIR)$(datadir)/mlt/jackrack"
	install -m 644 $(YML_FILES) "$(DESTDIR)$(datadir)/mlt/jackrack"
	[ -f $(BLACKLIST) ] && install -m 644 $(BLACKLIST) "$(DESTDIR)$(datadir)/mlt/jackrack" || true

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltjackrack$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(datadir)/mlt/jackrack"

ifneq ($(wildcard .depend),)
include .depend
endif
