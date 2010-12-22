CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak

TARGET = ../libmltdecklink$(LIBSUF)

OBJS = consumer_decklink.o \
	DeckLinkAPIDispatch.o

SRCS := $(OBJS:.o=.cpp)

CXXFLAGS += $(CFLAGS) -Wno-deprecated -Wno-multichar -fno-rtti

LDFLAGS += $(LIBDL)

all: 	$(TARGET)
	
$(TARGET): $(OBJS)
		$(CXX) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CXX) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
