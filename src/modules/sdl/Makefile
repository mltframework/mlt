include ../../../config.mak

include config.mak

TARGET = ../libmltsdl$(LIBSUF)

OBJS = factory.o \
	   consumer_sdl.o \
	   consumer_sdl_preview.o \
	   consumer_sdl_still.o 

ifeq ($(targetos),Darwin)
	CFLAGS +=-ObjC
	LDFLAGS +=-lobjc -framework Foundation
else
	LDFLAGS +=-lX11
endif

CFLAGS +=-I../.. `sdl-config --cflags`

LDFLAGS +=`sdl-config --libs`

LDFLAGS +=-L../../framework -lmlt

ifeq ($(WITH_SDL_IMAGE),1)
OBJS += producer_sdl_image.o
CFLAGS += -DWITH_SDL_IMAGE
LDFLAGS += -lSDL_image
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
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
