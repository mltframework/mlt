CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm -lpthread

include ../../../config.mak

TARGET = ../libmltplus$(LIBSUF)

OBJS = consumer_blipflash.o \
	   factory.o \
	   filter_affine.o \
	   filter_charcoal.o \
	   filter_dynamictext.o \
	   filter_invert.o \
	   filter_loudness.o \
	   filter_sepia.o \
	   producer_blipflash.o \
	   producer_count.o \
	   transition_affine.o \
	   ebur128/ebur128.o

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
	install -d "$(DESTDIR)$(mltdatadir)/plus"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/plus"

ifneq ($(wildcard .depend),)
include .depend
endif
