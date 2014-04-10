include ../../../config.mak
include config.mak

CFLAGS := -I../../ $(CFLAGS)
CXXFLAGS := $(CFLAGS) -Wno-deprecated $(CXXFLAGS)

LDFLAGS := -L../../framework -lmlt -lpthread $(LDFLAGS)

TARGET = ../libmltkino.so

OBJS = factory.o producer_kino.o
CPPOBJS = kino_wrapper.o avi.o error.o filehandler.o riff.o

LDFLAGS += -lstdc++

ifdef HAVE_LIBQUICKTIME
CFLAGS += `pkg-config --cflags libquicktime` 
CXXFLAGS += `pkg-config --cflags libquicktime` 
LDFLAGS += `pkg-config --libs libquicktime`
endif

ifdef HAVE_LIBDV
CFLAGS += `pkg-config --cflags libdv`
LDFLAGS += `pkg-config --libs libdv`
endif


SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cc)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CXX) -shared -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"

ifneq ($(wildcard .depend),)
include .depend
endif
