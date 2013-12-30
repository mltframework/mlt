CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltresample$(LIBSUF)

OBJS = factory.o \
	   filter_resample.o 

CFLAGS += $(shell pkg-config --cflags samplerate)

LDFLAGS += $(shell pkg-config --libs samplerate)

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
	install -d "$(DESTDIR)$(mltdatadir)/resample"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/resample"

ifneq ($(wildcard .depend),)
include .depend
endif
