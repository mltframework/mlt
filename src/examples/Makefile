CXXFLAGS += -Wall -g $(shell ${PKG_CONFIG} mlt++ --cflags)
LDFLAGS += $(shell ${PKG_CONFIG} mlt++ --libs)
CC=c++

all:		play

play:		play.o

play.o:		play.cpp

clean:
	$(RM) play play.o

distclean: clean

install:

uninstall:
