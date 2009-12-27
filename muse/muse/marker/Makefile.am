include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libmarker.a

dist_libmarker_a_SOURCES = \
	marker.cpp marker.h \
	markerview.cpp markerview.h

nodist_libmarker_a_SOURCES = \
	moc_markerview.cpp

