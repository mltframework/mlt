include ../../../config.mak

TARGET = ../libmltsox.so

OBJS = factory.o \
	   filter_sox.o 

CFLAGS += -I../../ -I/usr/include/sox

LDFLAGS += -lst -lpopt -lvorbis -logg -lvorbisfile -lvorbisenc

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
