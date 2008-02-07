include ../config.mak
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

OBJS = MltConsumer.o \
	   MltDeque.o \
	   MltEvent.o \
	   MltFactory.o \
	   MltField.o \
	   MltFilter.o \
	   MltFilteredConsumer.o \
	   MltFrame.o \
	   MltGeometry.o \
	   MltMiracle.o \
	   MltMultitrack.o \
	   MltParser.o \
	   MltPlaylist.o \
	   MltProducer.o \
	   MltProfile.o \
	   MltProperties.o \
	   MltPushConsumer.o \
	   MltRepository.o \
	   MltResponse.o \
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

clean:
	$(RM) $(OBJS) $(TARGET) $(NAME)

distclean:	clean

install:
	$(INSTALL) -d "$(DESTDIR)$(libdir)"
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(libdir)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(SONAME)
	$(INSTALL) -d "$(DESTDIR)$(prefix)/include/mlt++"
	$(INSTALL) -m 644 $(HEADERS) "$(DESTDIR)$(prefix)/include/mlt++"
	/sbin/ldconfig || true

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(NAME)"
	rm -f "$(DESTDIR)$(libdir)/$(SONAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt++"
