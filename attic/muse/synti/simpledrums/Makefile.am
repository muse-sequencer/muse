include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

AM_CXXFLAGS +=-fPIC -O3 -fno-rtti -ffast-math -fno-exceptions -include $(top_srcdir)/all-pic.h

synthi_LTLIBRARIES = simpledrums.la

simpledrums_la_SOURCES = simpledrums.h simpledrums.cpp common.h \
	simpledrumsgui.cpp simpledrumsgui.h \
	ssplugin.h ssplugin.cpp ssplugingui.h ssplugingui.cpp \
	simpledrumsguibase.ui sspluginchooserbase.ui

nodist_simpledrums_la_SOURCES = moc_simpledrumsgui.cpp moc_ssplugingui.cpp

simpledrums_la_LIBADD  = ../libsynti/libsynti.la -lsamplerate
simpledrums_la_LDFLAGS = -module -avoid-version

CLEANFILES = $(wildcard *.ui~)
EXTRA_DIST = README ReleaseNotes.txt COPYING

