CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak

include config.mak

TARGET = ../libmltrtaudio$(LIBSUF)

OBJS = consumer_rtaudio.o

SRCS := $(OBJS:.o=.cpp)

CXXFLAGS += $(CFLAGS) -Wno-deprecated -Wno-multichar -fno-rtti

ifdef USE_INTERNAL_RTAUDIO
OBJS += RtAudio.o
ifeq ($(targetos), Darwin)
CXXFLAGS += -D__MACOSX_CORE__
LDFLAGS += -framework CoreAudio -framework CoreFoundation
else ifeq ($(targetos), MinGW)
CXXFLAGS += -D__WINDOWS_DS__
LDFLAGS += -lole32 -ldsound -lwinmm
# For ASIO when ready to try that:
#OBJS += asio.o asiodrivers.o asiolist.o iasiothiscallresolver.o
#CXXFLAGS +=-D__WINDOWS_ASIO__
else ifeq ($(targetos), Linux)
CXXFLAGS += -D__LINUX_ALSA__
CXXFLAGS += $(shell pkg-config --cflags alsa)
LDFLAGS += $(shell pkg-config --libs alsa)
CXXFLAGS += -D__LINUX_PULSE__
CXXFLAGS += $(shell pkg-config --cflags libpulse libpulse-simple)
LDFLAGS += $(shell pkg-config --libs libpulse libpulse-simple)
else ifeq ($(targetos), NetBSD)
CXXFLAGS += -D__LINUX_OSS__
LDFLAGS += -lossaudio
else # FreeBSD
CXXFLAGS += -D__LINUX_OSS__
endif
endif # USE_INTERNAL_RTAUDIO

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CXX) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/rtaudio"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/rtaudio"

uninstall:
	rm -f "$(DESTDIR)$(moduledir)/libmltrtaudio$(LIBSUF)"
	rm -rf "$(DESTDIR)$(mltdatadir)/rtaudio"

ifneq ($(wildcard .depend),)
include .depend
endif
