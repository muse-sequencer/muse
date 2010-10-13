include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libcliplist.a

dist_libcliplist_a_SOURCES = \
	cliplist.cpp \
	cliplist.h

nodist_libcliplist_a_SOURCES = \
	moc_cliplist.cpp
