
TARGET = ../libmltgtk2.so

OBJS = factory.o \
	   producer_pixbuf.o \
	   producer_pango.o \
	   pixops.o \
	   filter_rescale.o

ASM_OBJS = have_mmx.o \
		   scale_line_22_yuv_mmx.o

CFLAGS = -O3 -DUSE_MMX `pkg-config gdk-pixbuf-2.0 --cflags` `pkg-config pangoft2 --cflags` -I../../ -Wall -g -D_FILE_OFFSET_BITS=64 -pthread

LDFLAGS = `pkg-config gdk-pixbuf-2.0 --libs` `pkg-config pangoft2 --libs`

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) -shared -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

have_mmx.o:
	$(CC) -o $@ -c have_mmx.S

scale_line_22_yuv_mmx.o: scale_line_22_yuv_mmx.S
	$(CC) -o $@ -c scale_line_22_yuv_mmx.S

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET)

ifneq ($(wildcard .depend),)
include .depend
endif
