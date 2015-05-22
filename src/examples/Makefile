CXXFLAGS += -Wall -g $(shell pkg-config mlt++ --cflags)
LDFLAGS += $(shell pkg-config mlt++ --libs-only-L)
LDLIBS += $(shell pkg-config mlt++ --libs-only-l)
CC=c++

all:		play

play:		play.o

play.o:		play.cpp

clean:
	$(RM) play play.o

distclean: clean

install:

uninstall:
