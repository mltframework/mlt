CFLAGS += -I../../

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltxine$(LIBSUF)

OBJS = factory.o \
	   deinterlace.o \
	   yadif.o \
	   filter_deinterlace.o

ifdef MMX_FLAGS
CFLAGS += -DARCH_X86
OBJS += cpu_accel.o
endif

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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"

ifneq ($(wildcard .depend),)
include .depend
endif
