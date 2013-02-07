CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltvorbis$(LIBSUF)

OBJS = factory.o \
	   producer_vorbis.o

CFLAGS += `pkg-config --cflags vorbis`
CFLAGS += `pkg-config --cflags vorbisfile`

LDFLAGS += `pkg-config --libs vorbis`
LDFLAGS += `pkg-config --libs vorbisfile`

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
	install -d "$(DESTDIR)$(mltdatadir)/vorbis"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/vorbis"

ifneq ($(wildcard .depend),)
include .depend
endif
