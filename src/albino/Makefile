include ../../config.mak

TARGET = albino

OBJS = albino.o

CFLAGS += -I.. -rdynamic

LDFLAGS = -L ../miracle -lmiracle 

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
