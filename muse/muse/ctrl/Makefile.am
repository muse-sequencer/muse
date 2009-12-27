include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libctrl.a

dist_libctrl_a_SOURCES = \
	ctrledit.cpp ctrledit.h \
	ctrlcanvas.cpp ctrlcanvas.h \
	ctrlpanel.cpp ctrlpanel.h

nodist_libctrl_a_SOURCES = \
	moc_ctrledit.cpp \
	moc_ctrlcanvas.cpp \
	moc_ctrlpanel.cpp
