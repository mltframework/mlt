CFLAGS += -I../..

LDFLAGS := -L../../framework -lmlt -lpthread -lm -L../../mlt++ -lmlt++ $(LDFLAGS)

include ../../../config.mak
include config.mak

TARGET = ../libmltkdenlive$(LIBSUF)

OBJS = factory.o \
	   filter_boxblur.o \
	   filter_freeze.o \
	   filter_wave.o \
	   producer_framebuffer.o

CPPOBJS += filter_typewriter.o \
	   typewriter.o

CXXFLAGS := $(QTCXXFLAGS) $(CXXFLAGS) $(CFLAGS) -Wno-deprecated

ifneq (, $(shell $(CXX) --version | grep -is -e g++ -e clang))
        _CXXFLAGS += -std=c++11
endif

LDFLAGS += $(QTLIBS)

SRCS := $(OBJS:.o=.c)
CPPSRCS := $(CPPOBJS:.o=.cpp)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CXX) $(SHFLAGS) -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS) $(CPPSRCS)
		$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

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
