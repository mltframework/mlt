CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm -lpthread

include ../../../config.mak

TARGET = ../libmltplusgpl$(LIBSUF)

OBJS = factory.o \
	   consumer_cbrts.o \
	   filter_burn.o \
	   filter_lumaliftgaingamma.o \
	   image.o \
	   utils.o \
	   filter_rotoscoping.o \
	   cJSON.o \
	   filter_telecide.o

ifeq ($(targetos), MinGW)
LDFLAGS += -lws2_32
else ifeq ($(targetos), Linux)
LDFLAGS += -lrt
endif

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
	install -d $(DESTDIR)$(mltdatadir)/plusgpl
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/plusgpl"

ifneq ($(wildcard .depend),)
include .depend
endif
