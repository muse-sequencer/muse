include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libmixer.a

dist_libmixer_a_SOURCES = \
	amixer.cpp amixer.h \
	meter.cpp meter.h \
	rack.cpp rack.h \
	panknob.cpp panknob.h \
	auxknob.cpp auxknob.h \
      routedialogbase.ui routedialog.h routedialog.cpp \
      strip.h strip.cpp astrip.h astrip.cpp mstrip.h mstrip.cpp

nodist_libmixer_a_SOURCES = \
	moc_amixer.cpp \
      moc_strip.cpp moc_astrip.cpp moc_mstrip.cpp \
	moc_meter.cpp \
	moc_rack.cpp \
	moc_panknob.cpp \
	moc_auxknob.cpp \
      moc_routedialog.cpp

