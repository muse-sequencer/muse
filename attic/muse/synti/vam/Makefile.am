include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

AM_CXXFLAGS += -O3 -ffast-math -fno-exceptions

synthi_LTLIBRARIES = vam.la

vam_la_SOURCES     = vam.cpp vam.h vamgui.cpp vamgui.h \
	               vamguibase.ui \
	               $(top_srcdir)/muse/xml.cpp

nodist_vam_la_SOURCES = moc_vamgui.cpp

vam_la_LIBADD  = ../libsynti/libsynti.la
vam_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = README TODO ChangeLog

