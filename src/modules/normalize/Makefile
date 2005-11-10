include ../../../config.mak

TARGET = ../libmltnormalize$(LIBSUF)

OBJS = factory.o \
	   filter_volume.o 

CFLAGS += -I../..

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
	install -m 755 $(TARGET) "$(DESTDIR)$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
