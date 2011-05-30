CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltplus$(LIBSUF)

OBJS = factory.o \
	   filter_affine.o \
	   filter_charcoal.o \
	   filter_invert.o \
	   filter_sepia.o \
	   transition_affine.o

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
	install -d "$(DESTDIR)$(datadir)/mlt/plus"
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/plus"

ifneq ($(wildcard .depend),)
include .depend
endif
