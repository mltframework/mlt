include ../../../config.mak

TARGET = ../libmltplus.so

OBJS = factory.o \
	   filter_charcoal.o \
	   filter_invert.o \
	   filter_sepia.o \
	   transition_affine.o

CFLAGS += -I../..

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
