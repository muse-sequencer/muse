include $(top_srcdir)/common.am
include $(top_srcdir)/synti/synti-install.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all-pic.h -DINSTPREFIX=\"$(prefix)\"
else
AM_CXXFLAGS += -DINSTPREFIX=\"$(prefix)\"
endif

synthi_LTLIBRARIES = deicsonze.la

deicsonze_la_SOURCES   = deicsonze.cpp deicsonze.h deicsonzegui.cpp deicsonzegui.h \
                     deicsonzeguibase.ui newpreset.ui

nodist_deicsonze_la_SOURCES = moc_deicsonzegui.cpp

deicsonze_la_LIBADD    = ../libsynti/libsynti.la
deicsonze_la_LDFLAGS   = -module -avoid-version

EXTRA_DIST = ARCH_ALIN README

deicsonzepkgdata_DATA = ARCH_ALIN
deicsonzepkgdatadir = $(pkgdatadir)/presets/deicsonze
