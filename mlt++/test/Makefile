include ../config.mak

CXXFLAGS+=-Wall -g `pkg-config mlt-framework --cflags` -I ../src
LDFLAGS+=-L../src -lmlt++ -lmiracle -lvalerie `pkg-config mlt-framework --libs`
CC=c++

all:		play server

play:		play.o

play.o:		play.cpp

server:		server.o

server.o:	server.cpp

clean:
	$(RM) play play.o
	$(RM) server server.o

distclean: clean

install:

uninstall:
