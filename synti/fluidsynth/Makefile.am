include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

AM_CXXFLAGS += -fPIC -O3 -ffast-math -fno-exceptions

synthi_LTLIBRARIES = fluidsynth.la

fluidsynth_la_SOURCES = fluidsynti.h fluidsynti.cpp \
	fluidsynthgui.cpp fluidsynthgui.h \
	fluidsynthguibase.ui

nodist_fluidsynth_la_SOURCES = moc_fluidsynthgui.cpp

fluidsynth_la_LIBADD  = ../libsynti/libsynti.la -lfluidsynth
fluidsynth_la_LDFLAGS = -module -avoid-version

CLEANFILES = $(wildcard *.ui~)
EXTRA_DIST = TODO README.txt

