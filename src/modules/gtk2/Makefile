include ../../../config.mak
include config.mak

CFLAGS := -I../.. $(CFLAGS)

LDFLAGS := -L../../framework -lmlt -lpthread -lm $(LDFLAGS)

TARGET = ../libmltgtk2$(LIBSUF)

OBJS = factory.o

ifdef USE_GTK2
OBJS += consumer_gtk2.o
CFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --cflags gtk+-2.0)
LDFLAGS += $(shell pkg-config $(PKGCONFIG_PREFIX) --libs gtk+-2.0)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/gtk2"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/gtk2"

ifneq ($(wildcard .depend),)
include .depend
endif
