include $(top_srcdir)/common.am

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS += -Imidiedit -Iarranger -Iliste \
	-Iwidgets -Imixer -Idriver -Iwaveedit -Implugins -Iinstruments -Iremote\
	-DINSTPREFIX=\"$(prefix)\"

SUBDIRS = widgets master midiedit mixer arranger \
	liste driver waveedit ctrl cliplist \
	instruments marker mplugins 

if PCH
AM_CXXFLAGS += $(LASH_CFLAGS) -include $(top_srcdir)/all.h
else
AM_CXXFLAGS += $(LASH_CFLAGS)
endif

if ENABLE_PYTHON
AM_CPPFLAGS += -DENABLE_PYTHON
SUBDIRS += remote
PYLIBS=-l$(PYTHON_LIB) remote/libremote.a
endif



EXTRA_PROGRAMS = givertcap
bin_PROGRAMS = muse @GIVERTCAP@

givertcap_LDADD = -lcap

dist_muse_SOURCES = \
      ticksynth.h ticksynth.cpp \
      vst.h vst.cpp \
      dssihost.h dssihost.cpp \
      synth.h synth.cpp  \
	plugin.cpp plugin.h \
	mtc.cpp mtc.h \
	thread.cpp thread.h \
	audio.cpp audio.h \
	audioprefetch.cpp audioprefetch.h \
	audioconvert.cpp audioconvert.h \
	globals.cpp globals.h \
      sync.cpp sync.h \
	midiport.cpp midiport.h \
	part.cpp part.h \
	tempo.cpp tempo.h \
	pos.cpp pos.h \
	sig.cpp sig.h \
	key.cpp key.h \
	undo.cpp undo.h \
	songfile.cpp \
	midi.cpp midi.h \
      importmidi.cpp \
      exportmidi.cpp \
      midifile.h midifile.cpp \
	xml.cpp xml.h \
	icons.cpp icons.h \
	event.cpp event.h eventbase.h evdata.h \
      eventlist.cpp \
      midievent.h midievent.cpp \
      waveevent.h waveevent.cpp \
	mpevent.cpp mpevent.h \
	track.cpp track.h \
	audiotrack.cpp wavetrack.cpp \
	wave.cpp wave.h \
	seqmsg.cpp \
	device.h \
	globaldefs.h \
	ladspa.h \
	app.cpp app.h \
	song.cpp song.h \
	transport.cpp transport.h \
	conf.cpp conf.h \
	confmport.cpp confmport.h \
	help.cpp \
	midieditor.cpp midieditor.h \
	cobject.cpp cobject.h \
	value.cpp value.h \
	midictrl.cpp midictrl.h \
	transpose.cpp transpose.h \
	miditransform.cpp miditransform.h \
	appearance.cpp appearance.h \
	node.cpp node.h \
	midiseq.cpp midiseq.h \
      helper.cpp helper.h \
	debug.h memory.h memory.cpp \
      mididev.h mididev.cpp \
      route.h route.cpp\
      shortcuts.h shortcuts.cpp \
      ctrl.cpp ctrl.h \
      gconfig.h gconfig.cpp \
      fastlog.h \
      default_click.h

nodist_muse_SOURCES = \
	moc_plugin.cpp \
	moc_app.cpp \
	moc_song.cpp \
	moc_transport.cpp \
	moc_conf.cpp \
	moc_confmport.cpp \
	moc_midieditor.cpp \
	moc_cobject.cpp \
	moc_value.cpp \
	moc_transpose.cpp \
	moc_miditransform.cpp \
	moc_appearance.cpp

muse_LDADD = $(X_LIBS) $(QT_LIBS) -lasound $(SNDFILE_LIBS) $(SAMPLERATE_LIBS) $(JACK_LIBS) $(PYLIBS) \
	$(LASH_LIBS) $(FST_LIBS) $(UUID_LIBS) $(LO_LIBS) midiedit/libmidiedit.a ctrl/libctrl.a \
	liste/libliste.a mixer/libmixer.a driver/libdriver.a marker/libmarker.a \
	master/libmaster.a waveedit/libwaveedit.a mplugins/libmplugins.a arranger/libarranger.a \
	cliplist/libcliplist.a instruments/libinstruments.a widgets/libwidgets.a \
	../synti/libsynti/libsynti.la ../al/libal.a
muse_LDFLAGS = -fno_exceptions 



CLEANFILES = stamp-chown

DISTCLEANFILES = .muse.bin.bak

MAINTAINERCLEANFILES = depcomp config.guess \
	install-sh config.sub missing mkinstalldirs INSTALL COPYING

EXTRA_DIST =

all-local: stamp-chown .muse.bin.bak

stamp-chown: muse @GIVERTCAP@
	if test "$(SUIDBUILD)" = "yes"; then \
	  su -c "chown root muse; chmod +s muse"; \
	fi
if RTCAP
	su -c "chown root givertcap; chmod +s givertcap";
endif
	touch stamp-chown

install-exec-local:
	if test "$(SUIDINSTALL)" = "yes"; then \
	  su -c "chown root $(bindir)/muse; chmod +s $(bindir)/muse"; \
	fi
if RTCAP
	su -c "chown root $(bindir)/givertcap; chmod +s $(bindir)/givertcap";
endif

update_translations:
	genpro > muse.pro
	lupdate muse.pro

release_translations:
	lrelease muse.pro

.muse.bin.bak:
	if test -e muse; then cp muse .muse.bin.bak; fi

clean-local:
	if test -e .muse.bin.bak; then cp .muse.bin.bak muse; fi


dox: stamp-doxygen

stamp-doxygen: $(shell find -name "*.cpp" -or -name "*.h")
	$(DOXYGEN) Doxyfile
	touch stamp-doxygen
