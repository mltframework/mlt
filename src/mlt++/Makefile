include ../../config.mak
include config.mak
INSTALL = install

ifneq ($(targetos), Darwin)
NAME = libmlt++$(LIBSUF)
TARGET = $(NAME).$(version)
SONAME = $(NAME).$(soversion)
LIBFLAGS += -Wl,-soname,$(SONAME)
else
NAME = libmlt++$(LIBSUF)
TARGET = libmlt++.$(version)$(LIBSUF)
SONAME = libmlt++.$(soversion)$(LIBSUF)
LIBFLAGS += -install_name $(libdir)/$(SONAME) -current_version $(version) -compatibility_version $(soversion)
endif

CXXFLAGS += -I.. $(RDYNAMIC) -DVERSION=\"$(version)\"

LDFLAGS += -L../framework -lmlt

OBJS = MltConsumer.o \
	   MltDeque.o \
	   MltEvent.o \
	   MltFactory.o \
	   MltField.o \
	   MltFilter.o \
	   MltFilteredConsumer.o \
	   MltFrame.o \
	   MltGeometry.o \
	   MltMultitrack.o \
	   MltParser.o \
	   MltPlaylist.o \
	   MltProducer.o \
	   MltProfile.o \
	   MltProperties.o \
	   MltPushConsumer.o \
	   MltRepository.o \
	   MltService.o \
	   MltTokeniser.o \
	   MltTractor.o \
	   MltTransition.o

SRCS = $(OBJS:.o=.cpp)
HEADERS = config.h Mlt.h $(OBJS:.o=.h)

all:		$(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) $(LIBFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	ln -sf $(TARGET) $(NAME)
	ln -sf $(TARGET) $(SONAME)

depend:	$(SRCS)
	$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

clean:
	$(RM) $(OBJS) $(TARGET) $(NAME) $(SONAME)

distclean:	clean

install:
	$(INSTALL) -d "$(DESTDIR)$(libdir)"
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(libdir)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(SONAME)
	$(INSTALL) -d "$(DESTDIR)$(prefix)/include/mlt++"
	$(INSTALL) -m 644 $(HEADERS) "$(DESTDIR)$(prefix)/include/mlt++"

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(NAME)"
	rm -f "$(DESTDIR)$(libdir)/$(SONAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt++"

ifneq ($(wildcard .depend),)
include .depend
endif
