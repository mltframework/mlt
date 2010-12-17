include ../../config.mak

NAME = libmlt$(LIBSUF)
TARGET = $(NAME).$(version)

ifneq ($(targetos), Darwin)
NAME = libmlt$(LIBSUF)
TARGET = $(NAME).$(version)
SONAME = $(NAME).$(soversion)
SHFLAGS += -Wl,-soname,$(SONAME)
else
NAME = libmlt$(LIBSUF)
TARGET = libmlt.$(version)$(LIBSUF)
SONAME = libmlt.$(soversion)$(LIBSUF)
SHFLAGS += -install_name $(libdir)/$(SONAME) -current_version $(version) -compatibility_version $(soversion)
endif

OBJS = mlt_frame.o \
	   mlt_version.o \
	   mlt_geometry.o \
	   mlt_deque.o \
	   mlt_property.o \
	   mlt_properties.o \
	   mlt_events.o \
	   mlt_parser.o \
	   mlt_service.o \
	   mlt_producer.o \
	   mlt_multitrack.o \
	   mlt_playlist.o \
	   mlt_consumer.o \
	   mlt_filter.o \
	   mlt_transition.o \
	   mlt_field.o \
	   mlt_tractor.o \
	   mlt_factory.o \
	   mlt_repository.o \
	   mlt_pool.o \
	   mlt_tokeniser.o \
	   mlt_profile.o \
	   mlt_log.o \
	   mlt_cache.o

INCS = mlt_consumer.h \
	   mlt_version.h \
	   mlt_factory.h \
	   mlt_filter.h \
	   mlt.h \
	   mlt_multitrack.h \
	   mlt_pool.h \
	   mlt_properties.h \
	   mlt_events.h \
	   mlt_parser.h \
	   mlt_repository.h \
	   mlt_tractor.h \
	   mlt_types.h \
	   mlt_deque.h \
	   mlt_field.h \
	   mlt_frame.h \
	   mlt_geometry.h \
	   mlt_playlist.h \
	   mlt_producer.h \
	   mlt_property.h \
	   mlt_service.h  \
	   mlt_transition.h \
	   mlt_tokeniser.h \
	   mlt_profile.h \
	   mlt_log.h \
	   mlt_cache.h

SRCS := $(OBJS:.o=.c)

CFLAGS += $(RDYNAMIC) -DPREFIX="\"$(prefix)\"" -DLIBDIR="\"$(libdir)\""

LDFLAGS += $(LIBDL) -lpthread

all:	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)
		ln -sf $(TARGET) $(NAME)
		ln -sf $(TARGET) $(SONAME)

depend:	$(SRCS)
	$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) $(NAME) $(SONAME)

install:
	install -d $(DESTDIR)$(libdir)
	install -m 755 $(TARGET) $(DESTDIR)$(libdir)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(SONAME)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME)
	install -d "$(DESTDIR)$(prefix)/include/mlt/framework"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/framework"
	install -d "$(DESTDIR)$(datadir)/mlt"
	install -m 644 metaschema.yaml "$(DESTDIR)$(datadir)/mlt/"

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(SONAME)"
	rm -f "$(DESTDIR)$(libdir)/$(NAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt/framework"
	rm -f "$(DESTDIR)$(datadir)/mlt/metaschema.yaml"

ifneq ($(wildcard .depend),)
include .depend
endif
