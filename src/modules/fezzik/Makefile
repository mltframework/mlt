include ../../../config.mak

TARGET = ../libmltfezzik.so

OBJS = factory.o \
	   producer_fezzik.o \
	   producer_hold.o 

CFLAGS += -I../..

LDFLAGS+=-L../../framework -lmlt

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
	install -m 644 ../fezzik.dict "$(prefix)/share/mlt/modules"
	install -m 644 ../fezzik.ini "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
