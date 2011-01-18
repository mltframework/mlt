CXXFLAGS += -Wall -g `pkg-config mlt++ --cflags` 
LDFLAGS += `pkg-config mlt++ --libs`
CC=c++

all:		play

play:		play.o

play.o:		play.cpp

clean:
	$(RM) play play.o

distclean: clean

install:

uninstall:
