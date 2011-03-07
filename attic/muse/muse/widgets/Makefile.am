include $(top_srcdir)/common.am

## musewidgetsplugin_so_CPPFLAGS = $(MUSECXXFLAGS) -fPIC -I.. -I$(top_srcdir)/synti \
##   -DQT_THREAD_SUPPORT -DQT_SHARED -DQT_PLUGIN
libwidgets_a_CPPFLAGS = $(MUSECXXFLAGS) -I.. -I$(top_srcdir)/synti 

if PCH
libwidgets_a_CPPFLAGS         += -include $(top_srcdir)/all.h
## musewidgetsplugin_so_CPPFLAGS += -include $(top_srcdir)/all-pic.h 
endif

AM_CXXFLAGS +=-DSVNVERSION=\"$(shell svnversion)\"


noinst_LIBRARIES = libwidgets.a

dist_libwidgets_a_SOURCES = \
	utils.cpp utils.h \
	citem.cpp citem.h \
	vscale.cpp vscale.h \
	drange.cpp drange.h \
	sclif.cpp sclif.h \
	dimap.cpp dimap.h \
	scldiv.cpp scldiv.h \
	mmath.cpp mmath.h \
	scldraw.cpp scldraw.h \
	action.h \
	\
	scrollscale.cpp scrollscale.h \
	view.cpp view.h \
	nentry.cpp nentry.h \
	mtscale.cpp mtscale.h \
	sigscale.cpp sigscale.h \
	hitscale.cpp hitscale.h \
	intlabel.cpp intlabel.h \
	doublelabel.cpp doublelabel.h \
	canvas.cpp canvas.h \
	noteinfo.cpp noteinfo.h \
	dentry.cpp dentry.h \
	siglabel.cpp siglabel.h \
	ttoolbar.cpp ttoolbar.h \
      ttoolbutton.cpp ttoolbutton.h \
	tb1.cpp tb1.h \
	lcombo.cpp lcombo.h \
	swidget.cpp swidget.h \
	tempolabel.cpp tempolabel.h \
	tools.cpp tools.h \
	fontsel.cpp fontsel.h \
	ctrlcombo.cpp ctrlcombo.h \
	sliderbase.cpp sliderbase.h \
	slider.cpp slider.h \
	midisyncimpl.cpp midisyncimpl.h \
	header.cpp header.h \
	knob.cpp knob.h \
	mlabel.cpp mlabel.h \
	genset.cpp genset.h \
	mixdowndialog.cpp mixdowndialog.h \
	spinboxFP.cpp spinboxFP.h \
	spinbox.cpp spinbox.h \
	comboQuant.cpp comboQuant.h \
	bigtime.cpp bigtime.h \
	comment.cpp comment.h \
	splitter.cpp splitter.h \
	gatetime.cpp gatetime.h \
	velocity.cpp velocity.h \
	metronome.cpp metronome.h \
	posedit.cpp posedit.h section.h \
	pitchedit.cpp pitchedit.h \
	pitchlabel.cpp pitchlabel.h \
	poslabel.cpp poslabel.h \
	filedialog.cpp filedialog.h \
	sigedit.cpp sigedit.h \
	shortcutconfig.cpp shortcutconfig.h \
	shortcutcapturedialog.cpp shortcutcapturedialog.h \
	combobox.cpp combobox.h \
	checkbox.cpp checkbox.h \
	aboutbox_impl.cpp aboutbox_impl.h \
	popupmenu.cpp popupmenu.h \
	listitem.h \
        menutitleitem.h \
	\
	midisync.ui \
	gensetbase.ui \
	mixdowndialogbase.ui \
	transposebase.ui \
	transformbase.ui \
	appearancebase.ui \
	itransformbase.ui \
	commentbase.ui \
	synthconfigbase.ui \
	gatetimebase.ui \
	velocitybase.ui \
	metronomebase.ui \
	mtrackinfobase.ui \
	wtrackinfobase.ui \
	editctrl7dialogbase.ui \
	editmetadialogbase.ui \
	editnotedialogbase.ui \
	editsysexdialogbase.ui \
	cliplisteditorbase.ui \
	mittransposebase.ui \
	fdialogbuttons.ui \
	shortcutconfigbase.ui \
	shortcutcapturedialogbase.ui \
    aboutbox.ui \
    songinfo.ui \
    didyouknow.ui \
      configmidifilebase.ui

nodist_libwidgets_a_SOURCES = \
	moc_scrollscale.cpp \
	moc_view.cpp \
	moc_nentry.cpp \
	moc_mtscale.cpp \
	moc_sigscale.cpp \
	moc_hitscale.cpp \
	moc_intlabel.cpp \
	moc_doublelabel.cpp \
	moc_canvas.cpp \
	moc_noteinfo.cpp \
	moc_dentry.cpp \
	moc_siglabel.cpp \
	moc_ttoolbar.cpp \
	moc_ttoolbutton.cpp \
	moc_tb1.cpp \
	moc_lcombo.cpp \
	moc_swidget.cpp \
	moc_tempolabel.cpp \
	moc_tools.cpp \
	moc_popupmenu.cpp \
	moc_fontsel.cpp \
	moc_ctrlcombo.cpp \
	moc_sliderbase.cpp \
	moc_slider.cpp \
	moc_midisyncimpl.cpp \
	moc_header.cpp \
	moc_knob.cpp \
	moc_mlabel.cpp \
	moc_genset.cpp \
	moc_mixdowndialog.cpp \
	moc_spinboxFP.cpp \
	moc_spinbox.cpp \
	moc_comboQuant.cpp \
	moc_bigtime.cpp \
	moc_comment.cpp \
	moc_splitter.cpp \
	moc_gatetime.cpp \
	moc_velocity.cpp \
	moc_metronome.cpp \
	moc_posedit.cpp \
	moc_pitchedit.cpp \
	moc_pitchlabel.cpp \
	moc_poslabel.cpp \
	moc_filedialog.cpp \
	moc_sigedit.cpp \
	moc_shortcutconfig.cpp \
	moc_shortcutcapturedialog.cpp \
      moc_combobox.cpp \
	moc_aboutbox_impl.cpp\
      moc_aboutbox.cpp \
      moc_songinfo.cpp \
      moc_didyouknow.cpp \
      moc_checkbox.cpp \
      moc_vscale.cpp \
      moc_action.cpp

# inst_PROGRAMS = musewidgetsplugin.so
# instdir = $(pkglibdir)/qtplugins/designer
#
### widgetsplugin_LTLIBRARIES = musewidgetsplugin.la
### widgetsplugindir = $(pkglibdir)/qtplugin

#dist_musewidgetsplugin_so_SOURCES = \
#	musewidgetsplug.cpp \
#	sigedit.h \
#	$(top_srcdir)/muse/xml.cpp $(top_srcdir)/muse/tempo.cpp \
#	$(top_srcdir)/muse/sig.cpp $(top_srcdir)/muse/pos.cpp

#nodist_musewidgetsplugin_so_SOURCES = \
#	moc_posedit.non-libtool.cpp moc_poslabel.non-libtool.cpp \
#	moc_sigedit.non-libtool.cpp \
#	moc_pitchedit.non-libtool.cpp moc_pitchlabel.non-libtool.cpp \
#	moc_tempolabel.non-libtool.cpp \
#	\
#	posedit.non-libtool.cpp poslabel.non-libtool.cpp \
#	pitchedit.non-libtool.cpp pitchlabel.non-libtool.cpp \
#	sigedit.non-libtool.cpp \
#	tempolabel.non-libtool.cpp \
#      slider.non-libtool.cpp moc_slider.non-libtool.cpp \
#      sliderbase.non-libtool.cpp moc_sliderbase.non-libtool.cpp \
#      sclif.non-libtool.cpp \
#      scldiv.non-libtool.cpp mmath.non-libtool.cpp scldraw.non-libtool.cpp \
#      dimap.non-libtool.cpp drange.non-libtool.cpp \
#      doublelabel.non-libtool.cpp moc_doublelabel.non-libtool.cpp \
#      dentry.non-libtool.cpp moc_dentry.non-libtool.cpp \
#      checkbox.non-libtool.cpp moc_checkbox.non-libtool.cpp \
#      combobox.non-libtool.cpp moc_combobox.non-libtool.cpp

#musewidgetsplugin_so_LDFLAGS = --whole-archive -fno-exceptions -shared \
#	-Wl,-soname,libmusewidgetsplugin.so.1 $(QT_LIBS) -lasound

#musewidgetsplug.o: musewidgetsplug.cpp
#	$(CXXCOMPILE) -fPIC -DQT_THREAD_SUPPORT -DQT_SHARED -DQT_PLUGIN -c musewidgetsplug.cpp

EXTRA_DIST = ctrlcombo.cw

# This is for stuff that needs the widgets, but dosn't build them with
# libtool

BUILT_SOURCES += \
	posedit.non-libtool.cpp \
	poslabel.non-libtool.cpp \
	pitchedit.non-libtool.cpp \
	pitchlabel.non-libtool.cpp \
	sigedit.non-libtool.cpp \
	tempolabel.non-libtool.cpp \
      slider.non-libtool.cpp \
      sliderbase.non-libtool.cpp \
      sclif.non-libtool.cpp \
	moc_posedit.non-libtool.cpp \
	moc_poslabel.non-libtool.cpp \
	moc_sigedit.non-libtool.cpp \
	moc_pitchedit.non-libtool.cpp \
	moc_pitchlabel.non-libtool.cpp \
	moc_tempolabel.non-libtool.cpp \
      moc_slider.non-libtool.cpp \
      moc_sliderbase.non-libtool.cpp \
      scldiv.non-libtool.cpp mmath.non-libtool.cpp scldraw.non-libtool.cpp \
      dimap.non-libtool.cpp drange.non-libtool.cpp \
      doublelabel.non-libtool.cpp moc_doublelabel.non-libtool.cpp \
      dentry.non-libtool.cpp moc_dentry.non-libtool.cpp \
      checkbox.non-libtool.cpp moc_checkbox.non-libtool.cpp \
      combobox.non-libtool.cpp moc_combobox.non-libtool.cpp

%.non-libtool.cpp: %.cpp
	$(LN_S) $*.cpp $*.non-libtool.cpp

CLEANFILES = \
	posedit.non-libtool.cpp \
	poslabel.non-libtool.cpp \
	pitchedit.non-libtool.cpp \
	pitchlabel.non-libtool.cpp \
	sigedit.non-libtool.cpp \
	tempolabel.non-libtool.cpp \
      slider.non-libtool.cpp \
      sliderbase.non-libtool.cpp \
      sclif.non-libtool.cpp \
	moc_posedit.non-libtool.cpp \
	moc_poslabel.non-libtool.cpp \
	moc_sigedit.non-libtool.cpp \
	moc_pitchedit.non-libtool.cpp \
	moc_pitchlabel.non-libtool.cpp \
	moc_tempolabel.non-libtool.cpp \
      moc_slider.non-libtool.cpp \
      moc_sliderbase.non-libtool.cpp \
      scldiv.non-libtool.cpp mmath.non-libtool.cpp scldraw.non-libtool.cpp \
      dimap.non-libtool.cpp drange.non-libtool.cpp \
      doublelabel.non-libtool.cpp moc_doublelabel.non-libtool.cpp \
      dentry.non-libtool.cpp moc_dentry.non-libtool.cpp \
      checkbox.non-libtool.cpp moc_checkbox.non-libtool.cpp \
      combobox.non-libtool.cpp moc_combobox.non-libtool.cpp

