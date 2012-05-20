CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lpthread

include ../../../config.mak

TARGET = ../libmltdecklink$(LIBSUF)

OBJS = consumer_decklink.o \
	producer_decklink.o \
	common.o

ifeq ($(targetos), MinGW)
CFLAGS += -Iwin
OBJS += win/DeckLinkAPI_i.o
LDFLAGS += -lole32
else
ifeq ($(targetos), Darwin)
CFLAGS += -Idarwin
OBJS += darwin/DeckLinkAPIDispatch.o
else
CFLAGS += -Ilinux
OBJS += linux/DeckLinkAPIDispatch.o
endif
endif

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
	install -d "$(DESTDIR)$(datadir)/mlt/decklink"
	install -m 644 *.yml "$(DESTDIR)$(datadir)/mlt/decklink"

uninstall:
	rm "$(DESTDIR)$(libdir)/mlt/libmltdecklink$(LIBSUF)" 2> /dev/null || true
	rm -rf "$(DESTDIR)$(datadir)/mlt/decklink"

ifneq ($(wildcard .depend),)
include .depend
endif
