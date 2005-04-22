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

CFLAGS += -I../.. `pkg-config --cflags jack`
CFLAGS += `xml2-config --cflags`
CFLAGS += `pkg-config glib-2.0 --cflags` 

LDFLAGS += `pkg-config --libs jack`
LDFLAGS += `xml2-config --libs`
LDFLAGS += `pkg-config glib-2.0 --libs`

LDFLAGS+=-L../../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
