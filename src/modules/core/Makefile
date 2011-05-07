CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm -lpthread

include ../../../config.mak

TARGET = ../libmltcore$(LIBSUF)

OBJS = factory.o \
	   producer_colour.o \
	   producer_consumer.o \
	   producer_hold.o \
	   producer_loader.o \
	   producer_noise.o \
	   producer_ppm.o \
	   filter_audiochannels.o \
	   filter_audioconvert.o \
	   filter_audiowave.o \
	   filter_brightness.o \
	   filter_channelcopy.o \
	   filter_crop.o \
	   filter_data_feed.o \
	   filter_data_show.o \
	   filter_gamma.o \
	   filter_greyscale.o \
	   filter_imageconvert.o \
	   filter_luma.o \
	   filter_mirror.o \
	   filter_mono.o \
	   filter_obscure.o \
	   filter_panner.o \
	   filter_region.o \
	   filter_rescale.o \
	   filter_resize.o \
	   filter_transition.o \
	   filter_watermark.o \
	   transition_composite.o \
	   transition_luma.o \
	   transition_mix.o \
	   transition_region.o \
	   consumer_null.o

ASM_OBJS = 

SRCS := $(OBJS:.o=.c)

ifeq ($(targetos), MinGW)
CFLAGS += -I../../win32
OBJS += ../../win32/fnmatch.o
SRCS += ../../win32/fnmatch.c
endif

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

composite_line_yuv_mmx.o: composite_line_yuv_mmx.S
	$(CC) -o $@ -c composite_line_yuv_mmx.S

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(prefix)/share/mlt/core"
	install -m 644 data_fx.properties "$(DESTDIR)$(datadir)/mlt/core"
	install -m 644 loader.dict "$(DESTDIR)$(datadir)/mlt/core"
	install -m 644 loader.ini "$(DESTDIR)$(datadir)/mlt/core"

ifneq ($(wildcard .depend),)
include .depend
endif
