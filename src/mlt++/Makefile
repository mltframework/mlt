include ../../config.mak
include config.mak
INSTALL = install

NAME = libmlt++$(LIBSUF)
ifeq ($(targetos), Darwin)
	TARGET = libmlt++.$(version)$(LIBSUF)
	SONAME = libmlt++.$(soversion)$(LIBSUF)
	LIBFLAGS += -install_name $(libdir)/$(SONAME) -current_version $(version) -compatibility_version $(soversion)
else ifeq ($(targetos), MinGW)
	TARGET = libmlt++-$(soversion)$(LIBSUF)
	CXXFLAGS += -DMLTPP_EXPORTS
	LIBFLAGS += -Wl,--output-def,libmlt++.def
else
	TARGET = $(NAME).$(version)
	SONAME = $(NAME).$(soversion)
	LIBFLAGS += -Wl,-soname,$(SONAME)
endif
ifneq (, $(shell $(CXX) --version | grep -is -e g++ -e clang))
	CXXFLAGS += -std=c++11
endif
CXXFLAGS += -I.. $(RDYNAMIC) -DVERSION=\"$(version)\" -fvisibility=hidden

LDFLAGS := -L../framework -lmlt $(LDFLAGS)
ifeq ($(targetos), Linux)
LDFLAGS += -Wl,--version-script=mlt++.vers
endif

OBJS = MltAudio.o \
	   MltAnimation.o \
	   MltConsumer.o \
	   MltDeque.o \
	   MltEvent.o \
	   MltFactory.o \
	   MltField.o \
	   MltFilter.o \
	   MltFilteredConsumer.o \
	   MltFilteredProducer.o \
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
HEADERS = MltConfig.h Mlt.h $(OBJS:.o=.h)

all:		$(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) $(LIBFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	ln -sf $(TARGET) $(NAME)
	if [ "$(targetos)" != "MinGW" ]; then \
		ln -sf $(TARGET) $(SONAME) ; \
	fi

depend:	$(SRCS)
	$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

clean:
	$(RM) $(OBJS) $(TARGET) $(NAME) $(SONAME) libmlt++.def

distclean:	clean

install:
	$(INSTALL) -d "$(DESTDIR)$(libdir)"
	if [ "$(targetos)" = "MinGW" ]; then \
		if [ "$(windeploy)" = true ]; then \
			$(INSTALL) -m 755 $(TARGET) "$(DESTDIR)$(prefix)" ; \
			$(INSTALL) -m 755 $(TARGET) "$(DESTDIR)$(libdir)/libmlt++.dll" ; \
		else \
			$(INSTALL) -m 755 $(TARGET) "$(DESTDIR)$(bindir)" ; \
			$(INSTALL) -m 755 $(TARGET) "$(DESTDIR)$(bindir)/libmlt++.dll" ; \
		fi; \
		$(INSTALL) -m 644 libmlt++.def "$(DESTDIR)$(libdir)" ; \
	else \
		$(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(libdir) ; \
		ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(SONAME) ; \
		ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME) ; \
	fi
	$(INSTALL) -d "$(DESTDIR)$(includedir)/mlt++"
	$(INSTALL) -m 644 $(HEADERS) "$(DESTDIR)$(includedir)/mlt++"

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(TARGET)"
	if [ "$(targetos)" != "MinGW" ]; then \
		rm -f "$(DESTDIR)$(libdir)/$(NAME)" ; \
		rm -f "$(DESTDIR)$(libdir)/$(SONAME)" ; \
	else \
		if [ "$(windeploy)" = true ]; then \
			rm -f "$(DESTDIR)$(prefix)/$(TARGET)" ; \
			rm -f "$(DESTDIR)$(libdir)/libmlt++.dll" ; \
		else \
			rm -f "$(DESTDIR)$(bindir)/$(TARGET)" ; \
			rm -f "$(DESTDIR)$(bindir)/libmlt++.dll" ; \
		fi ; \
		rm -f "$(DESTDIR)$(libdir)/libmlt++.def" ; \
	fi
	rm -rf "$(DESTDIR)$(includedir)/mlt++"

ifneq ($(wildcard .depend),)
include .depend
endif
