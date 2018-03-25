CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak

include config.mak

TARGET = ../libmltsdl2$(LIBSUF)

OBJS = factory.o \
	   common.o \
	   consumer_sdl2.o \
	   consumer_sdl2_audio.o

ifeq ($(targetos),Darwin)
CFLAGS += -ObjC
LDFLAGS += -lobjc -framework Foundation
else ifneq ($(targetos), MinGW)
LDFLAGS += -lX11
endif

ifeq ($(USE_PKG_CONFIG), 1)
CFLAGS += $(shell pkg-config --cflags sdl2)
LDFLAGS += $(shell pkg-config --libs sdl2)
else
CFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)
endif

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) consumer_sdl_audio.o $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/sdl2"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/sdl2"

ifneq ($(wildcard .depend),)
include .depend
endif
