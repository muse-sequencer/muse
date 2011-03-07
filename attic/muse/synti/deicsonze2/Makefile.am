include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all-pic.h -DINSTPREFIX=\"$(prefix)\"
else
AM_CXXFLAGS += -DINSTPREFIX=\"$(prefix)\"
endif

synthi_LTLIBRARIES = deicsonze2.la

deicsonze2_la_SOURCES   = deicsonze.cpp deicsonze.h \
                          deicsonzegui.cpp deicsonzegui.h \
                          deicsonzefilter.cpp deicsonzefilter.h \
                          deicsonzeplugin.cpp deicsonzeplugin.h \
                          deicsonzepreset.cpp deicsonzepreset.h \
                          deicsonzeguibase.h

nodist_deicsonze2_la_SOURCES = moc_deicsonzegui.cpp

deicsonze2_la_LIBADD    = ../libsynti/libsynti.la
deicsonze2_la_LDFLAGS   = -module -avoid-version

EXTRA_DIST = SutulaBank.dei README

deicsonzepkgdata_DATA = SutulaBank.dei
deicsonzepkgdatadir = $(pkgdatadir)/presets/deicsonze2
