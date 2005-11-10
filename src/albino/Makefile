include ../../config.mak

TARGET = albino

OBJS = albino.o

CFLAGS += -I.. $(RDYNAMIC)

LDFLAGS += -L../miracle -lmiracle -L../valerie -lvalerie -L../miracle -lmiracle -L../framework -lmlt

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
	install -d "$(DESTDIR)$(bindir)"
	install -c -s -m 755 $(TARGET) "$(DESTDIR)$(bindir)"

ifneq ($(wildcard .depend),)
include .depend
endif
