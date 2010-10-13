include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

AM_CXXFLAGS += -O3 -ffast-math -fno-exceptions

synthi_LTLIBRARIES = organ.la

organ_la_SOURCES   = organ.cpp organ.h organgui.cpp organgui.h \
                     organguibase.ui \
	               $(top_srcdir)/muse/xml.cpp

nodist_organ_la_SOURCES = moc_organgui.cpp

organ_la_LIBADD    = ../libsynti/libsynti.la
organ_la_LDFLAGS   = -module -avoid-version
