include ../config.mak

CXXFLAGS+=-Wall -g `pkg-config mlt-framework --cflags` -I ../src
LDFLAGS+=-L../src -lmlt++ `pkg-config mlt-framework --libs`
CC=c++

all:		play

play:		play.o

play.o:		play.cpp

clean:
	$(RM) play play.o

distclean: clean

install:

uninstall:
