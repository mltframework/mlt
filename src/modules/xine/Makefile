include ../../../config.mak

TARGET = ../libmltxine.so

OBJS = factory.o \
	   deinterlace.o \
	   cpu_accel.o \
	   filter_deinterlace.o

CFLAGS = -O4 -pipe -ffast-math -fomit-frame-pointer -I../../ -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -DARCH_X86

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
