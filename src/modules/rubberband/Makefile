CFLAGS += -I../..

LDFLAGS := -L../../framework -lmlt $(LDFLAGS)

include ../../../config.mak

TARGET = ../libmltrubberband$(LIBSUF)

OBJS = factory.o

CPPOBJS = filter_rbpitch.o 

CFLAGS += $(shell pkg-config --cflags rubberband)

CXXFLAGS += -Wno-deprecated $(CFLAGS)

LDFLAGS += $(shell pkg-config --libs rubberband)

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cpp)

all: $(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CXX) $(SHFLAGS) -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/rubberband"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/rubberband"

ifneq ($(wildcard .depend),)
include .depend
endif
