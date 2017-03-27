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
LDFLAGS += -lole32 -loleaut32
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
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/decklink"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/decklink"

uninstall:
	rm -f "$(DESTDIR)$(moduledir)/libmltdecklink$(LIBSUF)"
	rm -rf "$(DESTDIR)$(mltdatadir)/decklink"

ifneq ($(wildcard .depend),)
include .depend
endif
