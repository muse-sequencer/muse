include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libliste.a

dist_libliste_a_SOURCES = \
	listedit.cpp listedit.h \
	editevent.cpp editevent.h \
      editctrlbase.ui

nodist_libliste_a_SOURCES = \
	moc_listedit.cpp \
	moc_editevent.cpp
