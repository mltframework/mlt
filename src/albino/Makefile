include ../../config.mak

TARGET = albino

OBJS = albino.o

CFLAGS = -O3 -I .. -Wall -g -D_FILE_OFFSET_BITS=64 -pthread -rdynamic

LDFLAGS = -L ../valerie -L ../miracle -L ../framework -lmiracle -lmlt -lvalerie

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
