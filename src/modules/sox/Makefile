include ../../../config.mak

TARGET = ../libmltsox$(LIBSUF)

OBJS = factory.o \
	   filter_sox.o 

CFLAGS += -I../../ -I/usr/include/sox

LDFLAGS += -lst -lpopt -lvorbis -logg -lvorbisfile -lvorbisenc

LDFLAGS+=-L../../framework -lmlt -lmp3lame

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

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
