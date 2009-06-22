include ../../../config.mak

TARGET = ../libmltfrei0r$(LIBSUF)

OBJS = factory.o \
	   producer_frei0r.o \
	   filter_frei0r.o \
	   transition_frei0r.o \
	   frei0r_helper.o

CFLAGS += -I../..
CFLAGS += `pkg-config --cflags frei0r`

LDFLAGS += -L../../framework -lmlt
LDFLAGS += -lm $(LIBDL)
LDFLAGS += `pkg-config --libs frei0r`

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
	install -d "$(DESTDIR)$(prefix)/share/mlt/frei0r"
	install -m 644 blacklist.txt "$(DESTDIR)$(datadir)/mlt/frei0r"

ifneq ($(wildcard .depend),)
include .depend
endif
