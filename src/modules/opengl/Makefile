include ../../../config.mak

CFLAGS := -I../.. $(CFLAGS)

LDFLAGS := -L../../framework -lmlt -lm $(LDFLAGS)

TARGET = ../libmltopengl$(LIBSUF)

OBJS = factory.o

CPPOBJS = filter_glsl_manager.o
CPPOBJS += filter_movit_blur.o
CPPOBJS += filter_movit_convert.o
CPPOBJS += filter_movit_crop.o
CPPOBJS += filter_movit_deconvolution_sharpen.o
CPPOBJS += filter_movit_diffusion.o
CPPOBJS += filter_movit_glow.o
CPPOBJS += filter_movit_lift_gamma_gain.o
CPPOBJS += filter_movit_mirror.o
CPPOBJS += filter_movit_opacity.o
CPPOBJS += filter_movit_rect.o
CPPOBJS += filter_movit_resample.o
CPPOBJS += filter_movit_resize.o
CPPOBJS += filter_movit_saturation.o
CPPOBJS += filter_movit_vignette.o
CPPOBJS += filter_movit_white_balance.o
CPPOBJS += mlt_movit_input.o
CPPOBJS += transition_movit_luma.o
CPPOBJS += transition_movit_mix.o
CPPOBJS += transition_movit_overlay.o

CFLAGS += -Wno-deprecated
CFLAGS += $(shell pkg-config --cflags movit 2> /dev/null)
CXXFLAGS += $(CFLAGS)

SHADERDIR = $(shell pkg-config --variable=shaderdir movit)
CXXFLAGS += -DSHADERDIR=\"$(SHADERDIR)\"

LDFLAGS += -L../../mlt++ -lmlt++

ifeq ($(targetos), MinGW)
	CXXFLAGS += $(shell pkg-config --cflags movit)
	LDFLAGS += $(shell pkg-config --libs movit) -lopengl32 -lpthread
else
	LDFLAGS += $(shell pkg-config --libs movit 2> /dev/null)
ifeq ($(targetos), Darwin)
	CXXFLAGS += -FOpenGL
	LDFLAGS += -lpthread -framework OpenGL
else
	OBJS += consumer_xgl.o
	LDFLAGS += -lpthread -lGL -lX11
endif
endif

SRCS := $(OBJS:.o=.c) $(CPPOBJS:.o=.cpp)

all: $(TARGET)

$(TARGET): $(OBJS) $(CPPOBJS)
		$(CXX) $(SHFLAGS) -o $@ $(OBJS) $(CPPOBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CXX) -MM $(CXXFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend config.h config.mak

clean:	
		rm -f $(OBJS) $(TARGET) $(CPPOBJS)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/opengl"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/opengl"

ifneq ($(wildcard .depend),)
include .depend
endif
