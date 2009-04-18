include ../../../config.mak

TARGET = ../libmltresample$(LIBSUF)

OBJS = factory.o \
	   filter_resample.o 

CFLAGS += -I../..
CFLAGS += `pkg-config --cflags samplerate`

LDFLAGS += -L../../framework -lmlt
LDFLAGS += `pkg-config --libs samplerate`

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
