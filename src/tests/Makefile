TARGET = dan charlie

CFLAGS = -I .. -Wall -rdynamic -pthread

LDFLAGS = -L ../framework -lmlt 

all: $(TARGET)

dan:		dan.o 
			$(CC) dan.o -o $@ $(LDFLAGS)

charlie:	charlie.o 
			$(CC) charlie.o -o $@ $(LDFLAGS)

clean:
			rm -f dan.o charlie.o dan charlie

depend:		dan.c charlie.c
			$(CC) -MM $(CFLAGS) $^ 1>.depend

ifneq ($(wildcard .depend),)
include .depend
endif
