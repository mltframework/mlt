TARGET = inigo

OBJS = inigo.o \
	   io.o

CFLAGS = -O3 -I .. -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -rdynamic

LDFLAGS = -L ../framework -lmlt 

ifeq ($(MLT_GPROF),true)
CFLAGS+=-p
LDFLAGS+=-p
endif

SRCS := $(OBJS:.o=.c)

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

ifneq ($(wildcard .depend),)
include .depend
endif
