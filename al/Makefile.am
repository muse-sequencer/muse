include $(top_srcdir)/common.am

if PCH
AM_CXXFLAGS += -include $(top_srcdir)/all.h
endif

noinst_LIBRARIES = libal.a

dist_libal_a_SOURCES = \
	al.cpp al.h \
	dsp.cpp dsp.h

if USE_SSE
dist_libal_a_SOURCES += dspSSE.cpp dspXMM.cpp
dspSSE.o: dspSSE.cpp 
	$(CXXCOMPILE) -x assembler -c dspSSE.cpp
endif

