include config.mak

all clean distclean install uninstall:
	$(MAKE) DESTDIR=$(DESTDIR) -C src $@ 

dist:
	[ -d "mlt++-$(version)" ] && rm -rf "mlt++-$(version)" || echo -n
	svn export http://mlt.svn.sourceforge.net/svnroot/mlt/trunk/mlt++ "mlt++-$(version)"
	svn log > "mlt++-$(version)/ChangeLog"
	tar -cvzf "mlt++-$(version).tar.gz" "mlt++-$(version)"
