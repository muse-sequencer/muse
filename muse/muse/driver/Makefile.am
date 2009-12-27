include $(top_srcdir)/common.am

noinst_LIBRARIES = libdriver.a

AM_CXXFLAGS += $(JACK_CFLAGS)

libdriver_a_SOURCES = audiodev.h 	alsamidi.cpp alsamidi.h jack.cpp jackaudio.h \
		dummyaudio.cpp alsatimer.cpp alsatimer.h timerdev.h rtctimer.cpp rtctimer.h

