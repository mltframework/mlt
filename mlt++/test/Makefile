CXXFLAGS=-Wall `mlt-config --cflags` -I ../src
LDFLAGS=--L../src -lmlt++
CC=c++

all:	play

play:	play.o

play.o:	play.cpp

clean:
	$(RM) play play.o

install:
