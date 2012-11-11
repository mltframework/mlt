CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltnormalize$(LIBSUF)

OBJS = factory.o \
	   filter_audiolevel.o \
	   filter_volume.o 

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
	install -d "$(DESTDIR)$(datadir)/mlt/normalize"
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/normalize"

ifneq ($(wildcard .depend),)
include .depend
endif
