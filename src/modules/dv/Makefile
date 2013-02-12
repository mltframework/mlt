CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak

TARGET = ../libmltdv$(LIBSUF)

OBJS = factory.o \
	   producer_libdv.o \
	   consumer_libdv.o

CFLAGS += `pkg-config --cflags libdv`

LDFLAGS += `pkg-config --libs libdv`

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
	install -d "$(DESTDIR)$(mltdatadir)/dv"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/dv"

ifneq ($(wildcard .depend),)
include .depend
endif
