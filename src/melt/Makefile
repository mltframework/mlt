include ../../config.mak
include config.mak

OBJS = melt.o \
	   io.o

CFLAGS += -I.. $(RDYNAMIC) -DVERSION=\"$(version)\"

LDFLAGS += -L../framework -lmlt -lpthread

SRCS := $(OBJS:.o=.c)

ifeq ($(targetos), MinGW)
CFLAGS += `sdl-config --cflags`
LDFLAGS += `sdl-config --libs`
bindir = $(prefix)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install:	all
	install -d "$(DESTDIR)$(bindir)"
	install -c -m 755 $(TARGET) "$(DESTDIR)$(bindir)"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/$(TARGET)"

ifneq ($(wildcard .depend),)
include .depend
endif
