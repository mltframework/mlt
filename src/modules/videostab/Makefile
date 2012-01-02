include ../../../config.mak

CFLAGS += -I../..
ifdef SSE2_FLAGS
CFLAGS += -msse2
endif

LDFLAGS += -L../../framework -lmlt -lm


TARGET = ../libmltvideostab$(LIBSUF)

OBJS = factory.o \
	   filter_videostab.o filter_videostab2.o \
	   stabilize.o transform.o transform_image.o tlist.o\
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
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/videostab"


ifneq ($(wildcard .depend),)
include .depend
endif
