include ../../../config.mak

TARGET = ../libmltxml$(LIBSUF)

OBJS = factory.o \
	   consumer_xml.o \
	   producer_xml.o

CFLAGS += -I../..
CFLAGS += `pkg-config libxml-2.0 --cflags`

LDFLAGS += -L../../framework -lmlt
LDFLAGS += `pkg-config libxml-2.0 --libs`

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
	install -d "$(DESTDIR)$(datadir)/mlt/xml"
	install -m 644 mlt-xml.dtd "$(DESTDIR)$(datadir)/mlt/xml"

ifneq ($(wildcard .depend),)
include .depend
endif
