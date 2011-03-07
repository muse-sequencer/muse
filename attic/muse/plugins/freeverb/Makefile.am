include $(top_srcdir)/common.am
include $(top_srcdir)/plugins/plugins-install.am

AM_CXXFLAGS += -O3 -fno-exceptions -fno-rtti -ffast-math

plugins_LTLIBRARIES = freeverb.la

plugins_DATA =

freeverb_la_SOURCES = \
	freeverb.cpp \
	revmodel.cpp revmodel.h \
	allpass.h comb.h denormals.h tuning.h	
freeverb_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = readme.txt

