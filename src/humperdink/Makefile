include ../../config.mak

TARGET = humperdink

OBJS = client.o \
       io.o \
	   remote.o

CFLAGS = -O4 -pipe -ffast-math -fomit-frame-pointer -I .. -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -rdynamic

LDFLAGS = -L ../valerie -lvalerie

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

install:	all
	install -d "$(bindir)"
	install -c -s -m 755 $(TARGET) "$(bindir)"

ifneq ($(wildcard .depend),)
include .depend
endif
