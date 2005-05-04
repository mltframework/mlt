include ../../../config.mak
include config.mak

TARGET=../libmltkino.so

OBJS=factory.o producer_kino.o
CPPOBJS=kino_wrapper.o avi.o error.o filehandler.o riff.o
CFLAGS+=-I../../
LDFLAGS+=-L../../framework -lmlt -lstdc++
CXXFLAGS+=$(CFLAGS) -Wno-deprecated

ifdef HAVE_LIBQUICKTIME
CXXFLAGS+=`lqt-config --cflags` 
LDFLAGS+=`lqt-config --libs`
endif

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cc)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CC) -shared -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
