CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm -lpthread

include ../../../config.mak

TARGET = ../libmltavsync$(LIBSUF)

OBJS = factory.o \
	   producer_blipflash.o \
	   consumer_blipflash.o

ASM_OBJS = 

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS) $(ASM_OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(ASM_OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/avsync"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/avsync"

ifneq ($(wildcard .depend),)
include .depend
endif
