CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak
include config.mak

TARGET = ../libmltlinsys$(LIBSUF)

OBJS = factory.o \
	   consumer_SDIstream.o

ifdef WITH_JPEG
CFLAGS += -DWITH_JPEG
LDFLAGS += -ljpeg
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
	install -d "$(DESTDIR)$(mltdatadir)/linsys"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/linsys"

ifneq ($(wildcard .depend),)
include .depend
endif
