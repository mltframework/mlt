include ../../config.mak

TARGET = humperdink

OBJS = client.o \
       io.o \
	   remote.o

CFLAGS += -I.. -rdynamic

LDFLAGS += -L ../valerie -lvalerie

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
