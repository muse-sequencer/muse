include $(top_srcdir)/common.am
include $(top_srcdir)/plugins/plugins-install.am

AM_CXXFLAGS += -O3 -fno-exceptions -fno-rtti -ffast-math

plugins_LTLIBRARIES = pandelay.la

plugins_DATA =

pandelay_la_SOURCES = \
	ladspapandelay.cpp ladspapandelay.h \
	pandelay.cpp \
	pandelaymodel.cpp pandelaymodel.h	
pandelay_la_LDFLAGS = -module -avoid-version


