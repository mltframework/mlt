include ../config.mak
INSTALL = install

TARGET = libmlt++$(LIBSUF)

OBJS = MltConsumer.o \
	   MltDeque.o \
	   MltEvent.o \
	   MltFactory.o \
	   MltField.o \
	   MltFilter.o \
	   MltFilteredConsumer.o \
	   MltFrame.o \
	   MltGeometry.o \
	   MltMiracle.o \
	   MltMultitrack.o \
	   MltParser.o \
	   MltPlaylist.o \
	   MltProducer.o \
	   MltProperties.o \
	   MltPushConsumer.o \
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
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET)

install:
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(prefix)/lib
	$(INSTALL) -d "$(DESTDIR)$(prefix)/include/mlt++"
	$(INSTALL) -m 644 $(HEADERS) "$(DESTDIR)$(prefix)/include/mlt++"
	/sbin/ldconfig || true

