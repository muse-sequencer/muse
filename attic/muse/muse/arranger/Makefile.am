include $(top_srcdir)/common.am

noinst_LIBRARIES = libarranger.a

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

dist_libarranger_a_SOURCES = \
	arranger.cpp arranger.h \
	pcanvas.cpp pcanvas.h \
	tlist.cpp tlist.h \
	alayout.cpp alayout.h \
      trackinfo.cpp trackautomationview.h

nodist_libarranger_a_SOURCES = \
	moc_arranger.cpp \
	moc_pcanvas.cpp \
	moc_tlist.cpp \
      moc_alayout.cpp
