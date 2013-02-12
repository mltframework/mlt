CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm

include ../../../config.mak

TARGET = ../libmltoldfilm$(LIBSUF)

OBJS = factory.o \
	   filter_oldfilm.o \
	   filter_dust.o \
	   filter_lines.o \
	   filter_grain.o \
	   filter_tcolor.o \
	   filter_vignette.o

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
	install -d $(DESTDIR)$(mltdatadir)/oldfilm
	install -m 644 *.svg "$(DESTDIR)$(mltdatadir)/oldfilm"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/oldfilm"


ifneq ($(wildcard .depend),)
include .depend
endif
