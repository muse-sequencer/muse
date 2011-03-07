include $(top_srcdir)/common.am

noinst_LIBRARIES = libremote.a

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
else
AM_CXXFLAGS += -DINSTPREFIX=\"$(prefix)\"
endif

AM_CPPFLAGS += -I$(PYTHON_INCLUDES) -DENABLE_PYTHON

dist_libremote_a_SOURCES = \
	pyapi.cpp pyapi.h

