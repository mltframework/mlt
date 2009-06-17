include ../../config.mak

TARGET = mlt-process

OBJS = melt.o \
	   io.o

CFLAGS += -I.. $(RDYNAMIC) -DVERSION=\"$(version)\"

LDFLAGS += -L../framework -lmlt -lpthread

SRCS := $(OBJS:.o=.c)

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)
		ln -sf $@ melt

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install:	all
	install -d "$(DESTDIR)$(bindir)"
	install -c -s -m 755 $(TARGET) "$(DESTDIR)$(bindir)"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/$(TARGET)"
	rm -f "$(DESTDIR)$(bindir)/melt"

ifneq ($(wildcard .depend),)
include .depend
endif
