include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libinstruments.a

dist_libinstruments_a_SOURCES = \
	minstrument.cpp minstrument.h \
      editinstrument.h editinstrument.cpp \
	editinstrumentbase.ui 

nodist_libinstruments_a_SOURCES = \
      moc_editinstrument.cpp 
