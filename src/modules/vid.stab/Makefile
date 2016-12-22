CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltvidstab$(LIBSUF)

OBJS = factory.o \
	common.o

CPPOBJS = filter_deshake.o
CPPOBJS += filter_vidstab.o

CFLAGS += -Wno-deprecated
CFLAGS += $(shell pkg-config --cflags vidstab)
CXXFLAGS += $(CFLAGS)

LDFLAGS += -L../../mlt++ -lmlt++
LDFLAGS += $(shell pkg-config --libs vidstab)


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
	install -d "$(DESTDIR)$(mltdatadir)/vid.stab"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/vid.stab"

ifneq ($(wildcard .depend),)
include .depend
endif
