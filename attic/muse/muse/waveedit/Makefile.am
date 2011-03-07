include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libwaveedit.a

dist_libwaveedit_a_SOURCES = \
	waveedit.cpp waveedit.h \
	waveview.cpp waveview.h \
	editgainbase.ui \
	editgain.cpp editgain.h

nodist_libwaveedit_a_SOURCES = \
	moc_waveedit.cpp \
	moc_waveview.cpp \
	moc_editgain.cpp
