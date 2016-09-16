CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak
include config.mak

TARGET = ../libmltopencv$(LIBSUF)

OBJS = factory.o

CPPOBJS = filter_opencv_tracker.o

CXXFLAGS += -Wno-deprecated $(CFLAGS)

LDFLAGS += -L../../mlt++ -lmlt++

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
	install -d "$(DESTDIR)$(mltdatadir)/opencv"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/opencv"

ifneq ($(wildcard .depend),)
include .depend
endif
