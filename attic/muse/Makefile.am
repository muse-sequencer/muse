include $(top_srcdir)/common.am

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = m4 al synti muse lib share demos doc plugins grepmidi xpm packaging utils

DISTCLEANFILES = stamp-doxygen

MAINTAINERCLEANFILES = depcomp config.guess \
	install-sh config.sub missing mkinstalldirs INSTALL COPYING

BUILT_SOURCES += all-pic.h

if PCH
BUILT_SOURCES += all.h.gch all-pic.h.gch

all.h.gch: all.h
	$(CXX) $(AM_CXXFLAGS) all.h -o all.h.gch
all-pic.h.gch: all-pic.h
	$(CXX) $(AM_CXXFLAGS) -fPIC all-pic.h -o all-pic.h.gch
endif

all-pic.h: all.h
	cp all.h all-pic.h

EXTRA_DIST = common.am README.de README.softsynth \
	SECURITY autogen.sh README.translate README.shortcuts \
      README.instruments NEWS README.CVS muse.prj \
      README.ladspaguis all.h

dox: stamp-doxygen

stamp-doxygen: $(shell find -name "*.cpp" -or -name "*.h")
	$(DOXYGEN) Doxyfile
	touch stamp-doxygen
