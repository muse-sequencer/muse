include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libmaster.a

dist_libmaster_a_SOURCES = \
	lmaster.cpp lmaster.h \
	masteredit.cpp masteredit.h \
	master.cpp master.h \
	tscale.cpp tscale.h

nodist_libmaster_a_SOURCES = \
	moc_lmaster.cpp \
	moc_masteredit.cpp \
	moc_master.cpp \
	moc_tscale.cpp
