include $(top_srcdir)/common.am

noinst_LIBRARIES = libmplugins.a

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

dist_libmplugins_a_SOURCES = \
	mitplugin.cpp mitplugin.h \
	mittranspose.cpp mittranspose.h \
	midiitransform.cpp midiitransform.h \
	midifilterimpl.cpp midifilterimpl.h \
	mrconfig.cpp mrconfig.h \
	rhythm.cpp rhythm.h \
	random.cpp random.h \
	midifilter.ui \
	mrconfigbase.ui \
	rhythmbase.ui

nodist_libmplugins_a_SOURCES = \
	moc_mittranspose.cpp \
	moc_midiitransform.cpp \
	moc_midifilterimpl.cpp \
	moc_mrconfig.cpp \
	moc_rhythm.cpp
