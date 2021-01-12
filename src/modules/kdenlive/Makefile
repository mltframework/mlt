CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltkdenlive$(LIBSUF)

OBJS = factory.o \
	   filter_boxblur.o \
	   filter_freeze.o \
	   filter_wave.o \
	   producer_framebuffer.o


LDFLAGS += -lm

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
	install -d "$(DESTDIR)$(mltdatadir)/kdenlive"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/kdenlive"

ifneq ($(wildcard .depend),)
include .depend
endif
