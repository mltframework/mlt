include ../config.mak
CXXFLAGS = -Wall -fPIC -DPIC -pthread `mlt-config --cflags`
LDFLAGS = `mlt-config --libs` `mlt-config miracle --libs`
INSTALL = install

TARGET = libmlt++.so

OBJS = MltConsumer.o \
	   MltEvent.o \
	   MltFactory.o \
	   MltField.o \
	   MltFilter.o \
	   MltFilteredConsumer.o \
	   MltFrame.o \
	   MltMiracle.o \
	   MltMultitrack.o \
	   MltParser.o \
	   MltPlaylist.o \
	   MltProducer.o \
	   MltProperties.o \
	   MltResponse.o \
	   MltService.o \
	   MltTokeniser.o \
	   MltTractor.o \
	   MltTransition.o

SRCS = $(OBJS:.o=.cpp)

HEADERS = Mlt.h $(OBJS:.o=.h)

CC=g++

all:		$(TARGET)

$(TARGET):	$(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET)

install:
	$(INSTALL) -m 755 $(TARGET) $(prefix)/lib
	$(INSTALL) -d "$(prefix)/include/mlt++"
	$(INSTALL) -m 644 $(HEADERS) "$(prefix)/include/mlt++"
	/sbin/ldconfig || true

