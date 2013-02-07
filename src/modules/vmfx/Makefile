CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt

include ../../../config.mak

TARGET = ../libmltvmfx$(LIBSUF)

OBJS = factory.o \
	   filter_chroma.o \
	   filter_chroma_hold.o \
	   filter_mono.o \
	   filter_shape.o \
	   producer_pgm.o

SRCS := $(OBJS:.o=.c)

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
	install -d "$(DESTDIR)$(mltdatadir)/vmfx"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/vmfx"

ifneq ($(wildcard .depend),)
include .depend
endif
