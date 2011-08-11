CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltvideostab$(LIBSUF)

OBJS = factory.o \
	   filter_videostab.o \
		stab/klt/convolve.o           stab/klt/klt.o                stab/klt/pyramid.o            stab/klt/trackFeatures.o \
		stab/klt/error.o              stab/klt/klt_util.o           stab/klt/selectGoodFeatures.o \
		stab/estimate.o    stab/resample.o stab/utils.o    stab/vector.o

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
	install -d $(DESTDIR)$(datadir)/mlt/videostab
	install -m 644 *.svg "$(DESTDIR)$(datadir)/mlt/videostab"
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/videostab"


ifneq ($(wildcard .depend),)
include .depend
endif
