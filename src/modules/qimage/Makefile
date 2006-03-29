include ../../../config.mak
include config.mak

TARGET=../libmltqimage.so

OBJS=factory.o producer_qimage.o
CPPOBJS=qimage_wrapper.o
CFLAGS+=-I../../
LDFLAGS+=-L../../framework $(QTLIBS) -lmlt -lstdc++
CXXFLAGS+=$(CFLAGS) $(QTCXXFLAGS) -Wno-deprecated

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cpp)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CC) -shared -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $(QTCXXFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
