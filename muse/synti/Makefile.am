#if ENABLEFLUIDSYNTH
#SUBDIRS = libsynti s1 organ fluidsynth fluid vam deicsonze simpledrums
#else
#SUBDIRS = libsynti s1 organ vam deicsonze simpledrums
#endif
SUBDIRS = libsynti\
	s1 \
	organ \
	$(synth_fluidsynth) \
	$(synth_fluid) \
	vam \
	deicsonze \
	simpledrums
#	deicsonze2

include $(top_srcdir)/common.am

EXTRA_DIST = synti-install.am

