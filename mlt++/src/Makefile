include ../config.mak
CXXFLAGS = -Wall -pthread `mlt-config --cflags`
LDFLAGS = `mlt-config --libs`
INSTALL = install

TARGET = libmlt++.so

OBJS = MltConsumer.o \
	   MltFactory.o \
	   MltFilter.o \
	   MltFrame.o \
	   MltPlaylist.o \
	   MltProducer.o \
	   MltProperties.o \
	   MltService.o \
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
	/sbin/ldconfig || exit 0

