include ../../config.mak

NAME = libvalerie$(LIBSUF)
TARGET = libvalerie$(LIBSUF).$(version)

OBJS = valerie.o \
	   valerie_notifier.o \
	   valerie_parser.o \
	   valerie_response.o \
	   valerie_status.o \
	   valerie_tokeniser.o \
	   valerie_util.o \
	   valerie_remote.o \
	   valerie_socket.o

INCS = valerie.h \
	   valerie_notifier.h \
	   valerie_parser.h \
	   valerie_remote.h \
	   valerie_response.h \
	   valerie_socket.h \
	   valerie_status.h \
	   valerie_tokeniser.h \
	   valerie_util.h

SRCS := $(OBJS:.o=.c)

CFLAGS += -I.. $(RDYNAMIC)
LDFLAGS += -Wl,-soname,$(TARGET) -L../framework -lmlt -lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)
		ln -sf $(TARGET) $(NAME)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) $(NAME)

install: 	all
	install -m 755 $(TARGET) $(DESTDIR)$(libdir)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME)
	mkdir -p "$(DESTDIR)$(prefix)/include/mlt/valerie"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/valerie"

ifneq ($(wildcard .depend),)
include .depend
endif
