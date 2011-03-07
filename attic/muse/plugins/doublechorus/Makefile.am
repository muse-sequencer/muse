include $(top_srcdir)/common.am
include $(top_srcdir)/plugins/plugins-install.am

AM_CXXFLAGS += -O3 -fno-exceptions -fno-rtti -ffast-math

plugins_LTLIBRARIES = doublechorus.la

plugins_DATA =

doublechorus_la_SOURCES = \
	doublechorus.cpp \
	doublechorusmodel.cpp doublechorusmodel.h \
	simplechorusmodel.cpp simplechorusmodel.h
doublechorus_la_LDFLAGS = -module -avoid-version


