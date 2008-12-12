include ../../../config.mak

TARGET = ../libmltxine$(LIBSUF)

OBJS = factory.o \
	   deinterlace.o \
	   filter_deinterlace.o

ifdef MMX_FLAGS
OBJS += cpu_accel.o
endif

CFLAGS += -I../../ -DARCH_X86

LDFLAGS+=-L../../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
