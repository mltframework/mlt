include ../../../config.mak

TARGET = ../libmltfrei0r$(LIBSUF)

OBJS = factory.o \
	   producer_frei0r.o \
	   filter_frei0r.o \
	   transition_frei0r.o \
	   frei0r_helper.o

CFLAGS += -I../..

LDFLAGS += -lm

LDFLAGS+=-L../../framework -lmlt

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
