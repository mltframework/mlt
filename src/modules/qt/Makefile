include ../../../config.mak
include config.mak

CFLAGS := -I../.. $(CFLAGS)

LDFLAGS := -L../../framework -lmlt -lpthread -lm -L../../mlt++ -lmlt++ $(LDFLAGS)

TARGET = ../libmltqt$(LIBSUF)

OBJS = factory.o producer_qimage.o producer_kdenlivetitle.o
CPPOBJS = common.o \
	graph.o \
	filter_audiowaveform.o \
	qimage_wrapper.o \
	kdenlivetitle_wrapper.o \
	producer_qtext.o \
	transition_qtblend.o \
	filter_qtblend.o

ifdef USE_QT_OPENGL
	CPPOBJS += consumer_qglsl.o
	CFLAGS += -DUSE_QT_OPENGL
endif

ifdef GPL3
	CPPOBJS += transition_vqm.o
	CFLAGS += -DGPL3
endif

ifdef USE_FFTW
	CPPOBJS += filter_lightshow.o \
		filter_audiospectrum.o
	CFLAGS += -DUSE_FFTW
endif

ifneq ($(targetos), Darwin)
ifneq ($(targetos), MinGW)
    LDFLAGS += -lX11
endif
endif

CXXFLAGS := $(QTCXXFLAGS) $(CXXFLAGS) $(CFLAGS) $(EXIFCXXFLAGS) $(KDECXXFLAGS) -Wno-deprecated

ifneq (, $(shell $(CXX) --version | grep -is g++))
    CXXFLAGS += --std=c++11
endif

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
	install -d "$(DESTDIR)$(mltdatadir)/qt"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/qt"

ifneq ($(wildcard .depend),)
include .depend
endif
