include ../../../config.mak
include config.mak

TARGET = ../libmltsox$(LIBSUF)

OBJS = factory.o \
	   filter_sox.o 

CFLAGS += -I../../

LDFLAGS += -L../../framework -lmlt
# NOTE: Ubuntu may require -lsamplerate -lsndfile as well 
# since they are not in libst-config --libs

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
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
