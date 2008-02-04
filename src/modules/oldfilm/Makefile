include ../../../config.mak

TARGET = ../libmltoldfilm$(LIBSUF)

OBJS = factory.o \
	   filter_oldfilm.o \
	   filter_dust.o \
	   filter_lines.o \
	   filter_grain.o 

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
