include ../../../config.mak

TARGET = ../libmltmotion_est.so

OBJS = factory.o \
	   filter_motion_est.o \
	   filter_crop_detect.o \
	   filter_autotrack_rectangle.o \
	   arrow_code.o \
	   filter_vismv.o \
	   producer_slowmotion.o

CFLAGS += -I../.. 

LDFLAGS += -rdynamic

LDFLAGS+=-L../../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

dist-clean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(prefix)/share/mlt/modules"

test: $(TARGET)
	~/mlt-devel/mlt/src/inigo/inigo -filter motion_est -filter vismv -filter benchmark -consumer sdl rescale=none real_time=0 audio_off=1 silent=1 /media/cdrecorder/BBC.The.Private.Life.Of.Plants.Pt5.Living.Together.DivX505.AC3.www.MVGroup.org.uk.avi in=50000

hist: $(TARGET)
	~/mlt-devel/mlt/src/inigo/inigo -filter motion_est -filter histogram -consumer sdl rescale=none real_time=0 audio_off=1 silent=1 /media/cdrecorder/BBC.The.Private.Life.Of.Plants.Pt5.Living.Together.DivX505.AC3.www.MVGroup.org.uk.avi in=40000


test2: $(TARGET)
	inigo colour:black -filter watermark:"+mello.txt" composite.geometry="0,0:10%x10%;99=90%,90%" composite.out=99 -filter crop_detect -filter motion_est -filter vismv

realtime: $(TARGET)
	~/mlt-devel/mlt/src/inigo/inigo -filter motion_est -filter vismv -consumer sdl rescale=none /media/cdrecorder/BBC.The.Private.Life.Of.Plants.Pt5.Living.Together.DivX505.AC3.www.MVGroup.org.uk.avi in=30000

testhist: $(TARGET)
	~/mlt-devel/mlt/src/inigo/inigo -consumer sdl rescale=none silent=1 -filter motion_est -filter histogram  -filter vismv /media/cdrecorder/BBC.The.Private.Life.Of.Plants.Pt5.Living.Together.DivX505.AC3.www.MVGroup.org.uk.avi in=10000


ifneq ($(wildcard .depend),)
include .depend
endif
