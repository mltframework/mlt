CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltmelt$(LIBSUF)

OBJS = factory.o \
	   producer_melt.o 

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

ifneq ($(wildcard .depend),)
include .depend
endif
