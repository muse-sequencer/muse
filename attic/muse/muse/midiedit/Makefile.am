include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libmidiedit.a

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all-pic.h -DINSTPREFIX=\"$(prefix)\"
else
AM_CXXFLAGS += -DINSTPREFIX=\"$(prefix)\"
endif

dist_libmidiedit_a_SOURCES = \
	drummap.cpp drummap.h \
	cmd.h \
	pianoroll.cpp pianoroll.h \
	piano.cpp piano.h \
	prcanvas.cpp prcanvas.h \
	quantconfig.cpp quantconfig.h \
	drumedit.cpp drumedit.h \
	dlist.cpp dlist.h \
	dcanvas.cpp dcanvas.h \
	ecanvas.cpp ecanvas.h

nodist_libmidiedit_a_SOURCES = \
	moc_pianoroll.cpp \
	moc_piano.cpp \
	moc_prcanvas.cpp \
	moc_quantconfig.cpp \
	moc_drumedit.cpp \
	moc_dlist.cpp \
	moc_dcanvas.cpp \
	moc_ecanvas.cpp

