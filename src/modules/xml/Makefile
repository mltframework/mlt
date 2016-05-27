CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak

TARGET = ../libmltxml$(LIBSUF)

OBJS = factory.o \
	   consumer_xml.o \
	   producer_xml.o \
	   common.o

CFLAGS += $(shell pkg-config libxml-2.0 --cflags)

LDFLAGS += $(shell pkg-config libxml-2.0 --libs)

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
	install -d "$(DESTDIR)$(mltdatadir)/xml"
	install -m 644 mlt-xml.dtd "$(DESTDIR)$(mltdatadir)/xml"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/xml"

ifneq ($(wildcard .depend),)
include .depend
endif
