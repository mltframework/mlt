CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltvorbis$(LIBSUF)

OBJS = factory.o \
	   producer_vorbis.o

CFLAGS += $(shell ${PKG_CONFIG} --cflags vorbis)
CFLAGS += $(shell ${PKG_CONFIG} --cflags vorbisfile)

LDFLAGS += $(shell ${PKG_CONFIG} --libs vorbis)
LDFLAGS += $(shell ${PKG_CONFIG} --libs vorbisfile)

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
