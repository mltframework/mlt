include ../../../config.mak

TARGET = ../libmltsox.so

OBJS = factory.o \
	   filter_sox.o 

CFLAGS = -O4 -pipe -ffast-math -fomit-frame-pointer -I../../ -Wall -g \
	`libst-config --cflags`

LDFLAGS = `libst-config --libs` -lst

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
