include ../../../config.mak

TARGET = ../libmltxine.so

OBJS = factory.o \
	   deinterlace.o \
	   cpu_accel.o \
	   filter_deinterlace.o

CFLAGS += -I../../ -DARCH_X86

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

ifneq ($(wildcard .depend),)
include .depend
endif
