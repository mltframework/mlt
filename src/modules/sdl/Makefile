CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread -lm

include ../../../config.mak

include config.mak

TARGET = ../libmltsdl$(LIBSUF)

OBJS = factory.o \
	   consumer_sdl.o \
	   consumer_sdl_audio.o \
	   consumer_sdl_preview.o \
	   consumer_sdl_still.o 

ifeq ($(targetos),Darwin)
CFLAGS += -ObjC
LDFLAGS += -lobjc -framework Foundation
else ifneq ($(targetos), MinGW)
LDFLAGS += -lX11
endif

ifeq ($(USE_PKG_CONFIG), 1)
CFLAGS += $(shell pkg-config --cflags sdl)
LDFLAGS += $(shell pkg-config --libs sdl)
else
CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs)
endif

ifeq ($(WITH_SDL_IMAGE),1)
OBJS += producer_sdl_image.o
CFLAGS += -DWITH_SDL_IMAGE
LDFLAGS += -lSDL_image
endif

SRCS := $(OBJS:.o=.c)
ifeq ($(targetos),Darwin)
OBJS += consumer_sdl_osx.o
SRCS += consumer_sdl_osx.m consumer_sdl_osx.h
endif

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
	install -d "$(DESTDIR)$(mltdatadir)/sdl"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/sdl"

ifneq ($(wildcard .depend),)
include .depend
endif
