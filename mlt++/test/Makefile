include ../config.mak

CXXFLAGS+=-Wall -g `mlt-config --cflags` -I ../src
LDFLAGS+=-L../src -lmlt++ -lmiracle -lvalerie `mlt-config --libs`
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
