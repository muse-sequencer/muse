include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

AM_CXXFLAGS += -fPIC -O3 -ffast-math -fno-exceptions -fno-rtti

synthi_LTLIBRARIES = fluid.la

fluid_la_SOURCES = fluid.cpp fluid.h \
	fluidgui.cpp fluidgui.h \
	fluidguibase.ui

nodist_fluid_la_SOURCES = moc_fluidgui.cpp

fluid_la_LIBADD  = ../libsynti/libsynti.la -lfluidsynth
fluid_la_LDFLAGS = -module -avoid-version

