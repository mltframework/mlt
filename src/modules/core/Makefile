include ../../../config.mak

TARGET = ../libmltcore.so

OBJS = factory.o \
	   producer_colour.o \
	   producer_noise.o \
	   producer_ppm.o \
	   filter_brightness.o \
	   filter_channelcopy.o \
	   filter_gamma.o \
	   filter_greyscale.o \
	   filter_luma.o \
	   filter_mirror.o \
	   filter_obscure.o \
	   filter_region.o \
	   filter_resize.o \
	   filter_watermark.o \
	   transition_composite.o \
	   transition_luma.o \
	   transition_mix.o \
	   transition_region.o 

ASM_OBJS = composite_line_yuv_mmx.o

CFLAGS += -I../..

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) -shared -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

composite_line_yuv_mmx.o: composite_line_yuv_mmx.S
	$(CC) -o $@ -c composite_line_yuv_mmx.S

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

ifneq ($(wildcard .depend),)
include .depend
endif
