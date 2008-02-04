include ../../../config.mak
include config.mak

TARGET=../libmltkino.so

OBJS=factory.o producer_kino.o
CPPOBJS=kino_wrapper.o avi.o error.o filehandler.o riff.o
CFLAGS+=-I../../
LDFLAGS+=-L../../framework -lmlt -lstdc++
CXXFLAGS+=$(CFLAGS) -Wno-deprecated

ifdef HAVE_LIBQUICKTIME
CFLAGS+=`pkg-config --cflags libquicktime` 
CXXFLAGS+=`pkg-config --cflags libquicktime` 
LDFLAGS+=`pkg-config --libs libquicktime`
endif

ifdef HAVE_LIBDV
CFLAGS += `pkg-config --cflags libdv`
LDFLAGS += `pkg-config --libs libdv`
endif


SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cc)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CC) -shared -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
