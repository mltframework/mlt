CFLAGS += -fPIC -I../.. -Wno-deprecated -Wno-multichar
LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak
include config.mak

TARGET = ../libmltndi$(LIBSUF)

OBJS = factory.o consumer_ndi.o producer_ndi.o

SRCS := $(OBJS:.o=.c)

LDFLAGS += $(LIBDL)

all: 	$(TARGET)
	
$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/ndi"
#	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/ndi"

uninstall:
	rm -f "$(DESTDIR)$(moduledir)/libmltndi$(LIBSUF)"
	rm -rf "$(DESTDIR)$(mltdatadir)/ndi"

ifneq ($(wildcard .depend),)
include .depend
endif
