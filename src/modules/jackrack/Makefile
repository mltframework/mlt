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
	producer_ladspa.o \
	filter_jackrack.o \
	filter_ladspa.o

OBJS = factory.o \
	   consumer_jack.o

CFLAGS += $(shell pkg-config --cflags jack)
LDFLAGS += $(shell pkg-config --libs jack)

ifdef GPL
OBJS += $(GPL_OBJS)
CFLAGS += -DGPL
CFLAGS += $(shell pkg-config --cflags libxml-2.0)
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags glib-2.0)

LDFLAGS += $(LIBDL)
LDFLAGS += $(shell pkg-config --libs libxml-2.0)
LDFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --libs glib-2.0)
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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/jackrack"
	install -m 644 $(YML_FILES) "$(DESTDIR)$(mltdatadir)/jackrack"
	[ -f $(BLACKLIST) ] && install -m 644 $(BLACKLIST) "$(DESTDIR)$(mltdatadir)/jackrack" || true

uninstall:
	rm -f "$(DESTDIR)$(moduledir)/libmltjackrack$(LIBSUF)"
	rm -rf "$(DESTDIR)$(mltdatadir)/jackrack"

ifneq ($(wildcard .depend),)
include .depend
endif
