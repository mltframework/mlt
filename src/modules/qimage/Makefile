CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak
include config.mak

TARGET = ../libmltqimage$(LIBSUF)

OBJS = factory.o producer_qimage.o
CPPOBJS = qimage_wrapper.o

CXXFLAGS += $(CFLAGS) $(QTCXXFLAGS) -Wno-deprecated

LDFLAGS += $(QTLIBS)
LDFLAGS += -lstdc++

ifdef USE_KDE
LDFLAGS += -lkio
endif

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cpp)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $(QTCXXFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
