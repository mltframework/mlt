TARGET = dan charlie pango pixbuf dissolve luma

CFLAGS = -O3 -I .. -Wall -rdynamic -pthread

LDFLAGS = -L ../framework -L ../modules -lmlt -lmltdv -lmltsdl

ifeq ($(MLT_GPROF),true)
CFLAGS+=-p
LDFLAGS+=-p
endif

all: $(TARGET)

pango:		pango.o
			$(CC) pango.o -o $@ $(LDFLAGS)

pixbuf:		pixbuf.o
			$(CC) pixbuf.o -o $@ $(LDFLAGS)

dissolve:		dissolve.o
			$(CC) dissolve.o -o $@ $(LDFLAGS)

luma:		luma.o
			$(CC) luma.o -o $@ $(LDFLAGS)

dan:		dan.o 
			$(CC) dan.o -o $@ $(LDFLAGS)

charlie:	charlie.o io.o
			$(CC) charlie.o io.o -o $@ $(LDFLAGS)

clean:
			rm -f dan.o io.o charlie.o dan charlie

depend:		dan.c charlie.c io.c
			$(CC) -MM $(CFLAGS) $^ 1>.depend

ifneq ($(wildcard .depend),)
include .depend
endif
