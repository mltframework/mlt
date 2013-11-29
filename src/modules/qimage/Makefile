CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm -L../../mlt++ -lmlt++

include ../../../config.mak
include config.mak

TARGET = ../libmltqimage$(LIBSUF)

OBJS = factory.o producer_qimage.o producer_kdenlivetitle.o
CPPOBJS = qimage_wrapper.o \
	kdenlivetitle_wrapper.o \
	consumer_qglsl.o \
	producer_qtext.o

ifdef GPL3
	CPPOBJS += transition_vqm.o
	CFLAGS += -DGPL3
endif

ifneq ($(targetos), Darwin)
ifneq ($(targetos), MinGW)
    LDFLAGS += -lX11
endif
endif

CXXFLAGS := $(QTCXXFLAGS) $(CXXFLAGS) $(CFLAGS) $(EXIFCXXFLAGS) $(KDECXXFLAGS) -Wno-deprecated

LDFLAGS += $(QTLIBS) $(EXIFLIBS) $(KDELIBS)

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cpp)

all: 	$(TARGET)

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
	install -d "$(DESTDIR)$(mltdatadir)/qimage"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/qimage"

ifneq ($(wildcard .depend),)
include .depend
endif
