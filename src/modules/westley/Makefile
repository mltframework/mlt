include ../../../config.mak

TARGET = ../libmltwestley.so

OBJS = factory.o \
	   consumer_westley.o \
	   producer_westley.o

CFLAGS += -I../../ `xml2-config --cflags`

LDFLAGS += `xml2-config --libs`

LDFLAGS+=-L../../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"
	install -m 644 westley.dtd "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
