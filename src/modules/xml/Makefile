include ../../../config.mak

TARGET = ../libmltwestley$(LIBSUF)

OBJS = factory.o \
	   consumer_westley.o \
	   producer_westley.o

CFLAGS += -I../..
CFLAGS += `xml2-config --cflags`

LDFLAGS += -L../../framework -lmlt
LDFLAGS += `xml2-config --libs`

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
	install -m 644 westley.dtd "$(DESTDIR)$(prefix)/share/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
