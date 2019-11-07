CXX ?= g++
SWIG ?= swig
PYTHON ?= python3
PYTHON_BIN := $(shell which $(PYTHON))
PYTHON_PKGDIR ?= $(shell $(PYTHON) -c "import site; print(site.getsitepackages()[0])")
PYTHON_INCLUDE ?= $(shell "${PYTHON}-config" --includes)
PYTHON_LDFALGS ?= $(shell "${PYTHON}-config" --ldflags)
CXXFLAGS ?= -O2 -g

all:
	ln -sf ../mlt.i
	$(SWIG) -c++ -I../../mlt++ -I../.. -python mlt.i
	# Compile the wrapper
	$(CXX) -fPIC -D_GNU_SOURCE $(CXXFLAGS) -c -I../.. -I$(PYTHON_INCLUDE) mlt_wrap.cxx
	# Create the module
	$(CXX) $(CXXFLAGS) -shared mlt_wrap.o -L../../mlt++ -lmlt++ -L../../framework -lmlt $(PYTHON_LDFLAGS) -o _mlt.so

install:
	mkdir -p "$(DESTDIR)/$(PYTHON_PKGDIR)"
	# Replace existing shebang with a path to python executable
	# if it exists in a file being installed
	for i in $(shell find . -name 'mlt.py*'); do \
		if head -n 1 $$i | grep -qE '(^#!/|^#! /|^# !/)' ; \
			then \
				cat $$i | sed "1 s,^.*$,#\!$(PYTHON_BIN)," > $(DESTDIR)/$(PYTHON_PKGDIR)/$$i \
				chmod 0644 $(DESTDIR)/$(PYTHON_PKGDIR)/$$i ; \
			else \
				install -m 0644 $$i $(DESTDIR)/$(PYTHON_PKGDIR)/ ; \
		fi ; \
	done
	install -Dm 0755 _mlt.so $(DESTDIR)/$(PYTHON_PKGDIR)/

clean:
	rm -fv *.cxx *.so *.o mlt.i ../.python mlt.py*
