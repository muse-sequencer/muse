#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2008 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

include(${PROJECT_SOURCE_DIR}/pch.txt)

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

set (SubDirs 
     widgets master midiedit mixer arranger 
     liste driver waveedit ctrl cliplist 
     instruments marker mplugins      
    )

if (ENABLE_PYTHON)
set ( PYREM_CPP_FLAGS "-DENABLE_PYTHON" )
set ( PYLIBS ${PYTHON_LIB} remote) 
set ( SubDirs ${SubDirs} remote )
endif (ENABLE_PYTHON)

subdirs (${SubDirs})

#QT4_WRAP_UI  (muse_ui_headers
# QT4_WRAP_UI3  (muse_ui_headers
#       preferences.ui
#       exportmididialog.ui
#       configmidifile.ui
#       transport.ui
#       aboutbox.ui
#       transpose.ui
#       projectpropsdialog.ui
#       projectdialog.ui
#       templatedialog.ui
#       importmidi.ui
#       routedialog.ui
#       configtrack.ui
#       midiportinfo.ui
#       miditrackinfo.ui
#       )

QT4_WRAP_CPP ( muse_moc_headers
#       projectpropsdialog.h
# 	projectdialog.h
#       templatedialog.h
#       preferences.h
#       plugingui.h
#       midiplugin.h
#       muse.h
#       song.h
#       transport.h
#       conf.h
#       editor.h
# 	cobject.h
#       transpose.h
#       track.h
#       miditrackbase.h
#       midisynti.h
#       miditrack.h
#       wavetrack.h
#       audiotrack.h
#       audiooutput.h
#       audioinput.h
#       audiogroup.h
#       synth.h
#       exportmidi.h
#       importmidi.h
#       midiinport.h
#       midioutport.h
#       astrip.h
#       midirack.h
#       mixer.h
#       mstrip.h
#       rack.h
#       routedialog.h
#       strip.h
#       arranger.h
#       canvas.h
#       configtrack.h
#       partdrag.h
#       tllineedit.h
#       tlswidget.h
#       tlwidget.h
#       trackdrag.h
#       trackinfo.h
      
      plugin.h 
      app.h 
      song.h 
      transport.h 
      conf.h 
      confmport.h 
      midieditor.h 
      cobject.h 
      value.h 
      transpose.h 
      miditransform.h 
      appearance.h
      )

QT4_ADD_RESOURCES (muse_qrc_files muse.qrc)

add_executable ( muse
      ${muse_qrc_files}
      ${muse_moc_headers}
      # ${muse_ui_headers}
      ${PROJECT_BINARY_DIR}/all.h.pch

#       ticksynth.cpp
#       vst.cpp
#       dssihost.cpp
#       synth.cpp
#       plugin.cpp
#       ladspaplugin.cpp
#       auxplugin.cpp
#       pipeline.cpp
# 	mtc.cpp
#       thread.cpp
#       audio.cpp
# 	audioprefetch.cpp
#       audiowriteback.cpp
#       globals.cpp
#       sync.cpp
# 	part.cpp
#       undo.cpp
#       songfile.cpp
#       midi.cpp
#       importmidi.cpp
#       importmidi.h
#       midifile.cpp
#       icons.cpp
#       event.cpp
#       eventlist.cpp
#       midieventbase.cpp
#       waveevent.cpp
#       midievent.cpp
# 	wave.cpp
#       seqmsg.cpp
# 	help.cpp
# 	midictrl.cpp
#       fifo.cpp
#       helper.cpp
#       route.cpp
#       shortcuts.cpp
#       ctrl.cpp
#       gconfig.cpp
#       midirc.cpp
#       midififo.cpp
#       midioutport.cpp
#       midiinport.cpp
#       midiout.cpp
# 
#       projectpropsdialog.cpp
# 	projectdialog.cpp
#       templatedialog.cpp
#       preferences.cpp
#       plugingui.cpp
#       midiplugin.cpp
#       muse.cpp
#       song.cpp
#       songpart.cpp
#       songtrack.cpp
#       transport.cpp
#       conf.cpp
#       editor.cpp
# 	cobject.cpp
#       transpose.cpp
#       track.cpp
#       miditrackbase.cpp
#       midisynti.cpp
#       miditrack.cpp
#       wavetrack.cpp
#       audiotrack.cpp
#       audiooutput.cpp
#       audioinput.cpp
#       audiogroup.cpp
#       exportmidi.cpp
#       revision.cpp
#       jack.cpp dummyaudio.cpp
#       astrip.cpp
#       midirack.cpp
#       mixer.cpp
#       mstrip.cpp
#       rack.cpp
#       routedialog.cpp
#       strip.cpp
# 
#       arranger.cpp
#       canvas.cpp
#       configtrack.cpp
#       partdrag.cpp
#       tllineedit.cpp
#       tlswidget.cpp
#       tlwidget.cpp
#       tlwlayout.cpp
#       trackdrag.cpp
#       trackinfo.cpp
      
      ticksynth.h ticksynth.cpp 
      vst.h vst.cpp 
      dssihost.h dssihost.cpp 
      osc.cpp osc.h 
      stringparam.cpp stringparam.h 
      synth.h synth.cpp  
      plugin.cpp plugin.h 
      mtc.cpp mtc.h 
      thread.cpp thread.h 
      audio.cpp audio.h 
      audioprefetch.cpp audioprefetch.h 
      audioconvert.cpp audioconvert.h 
      globals.cpp globals.h 
      sync.cpp sync.h 
      midiport.cpp midiport.h 
      part.cpp part.h 
      tempo.cpp tempo.h 
      pos.cpp pos.h 
      sig.cpp sig.h 
      key.cpp key.h 
      undo.cpp undo.h 
      songfile.cpp 
      midi.cpp midi.h 
      importmidi.cpp 
      exportmidi.cpp 
      midifile.h midifile.cpp 
      xml.cpp xml.h 
      icons.cpp icons.h 
      event.cpp event.h eventbase.h evdata.h 
      eventlist.cpp 
      midievent.h midievent.cpp 
      waveevent.h waveevent.cpp 
      mpevent.cpp mpevent.h 
      track.cpp track.h 
      audiotrack.cpp wavetrack.cpp 
      wave.cpp wave.h 
      seqmsg.cpp 
      device.h 
      globaldefs.h 
      ladspa.h 
      app.cpp app.h 
      song.cpp song.h 
      transport.cpp transport.h 
      conf.cpp conf.h 
      confmport.cpp confmport.h 
      help.cpp 
      midieditor.cpp midieditor.h 
      cobject.cpp cobject.h 
      value.cpp value.h 
      midictrl.cpp midictrl.h 
      transpose.cpp transpose.h 
      miditransform.cpp miditransform.h 
      appearance.cpp appearance.h 
      node.cpp node.h 
      midiseq.cpp midiseq.h 
      helper.cpp helper.h 
      debug.h memory.h memory.cpp 
      mididev.h mididev.cpp 
      route.h route.cpp
      shortcuts.h shortcuts.cpp 
      ctrl.cpp ctrl.h 
      gconfig.h gconfig.cpp 
      fastlog.h 
      default_click.h
      )

# AM_CXXFLAGS +=-DSVNVERSION=\"$(shell svnversion)\"
EXEC_PROGRAM( svnversion
      ARGS ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE SVNVER )

# add_definitions ( -DINSTLIBDIR=${LIBRARY_OUTPUT_DIRECTORY} )
include_directories (midiedit arranger liste widgets mixer driver waveedit mplugins instruments remote)

# For some reason this was needed to find routedialogbase.h.
include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/mixer
      )

set_target_properties( muse
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      # PROPERTIES COMPILE_FLAGS "-Imidiedit -Iarranger -Iliste -Iwidgets -Imixer -Idriver -Iwaveedit -Implugins -Iinstruments -Iremote ${PYREM_CPP_FLAGS} -DSVNVERSION='\"${SVNVER}\"' -include ${PROJECT_BINARY_DIR}/all.h "
      PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -DINSTPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DINSTLIBDIR='\"${LIBRARY_OUTPUT_DIRECTORY}\"' -DSVNVERSION='\"${SVNVER}\"' -include ${PROJECT_BINARY_DIR}/all.h "
      )

target_link_libraries(muse
#       midiedit
#       master
#       instruments
#       marker
#       liste
#       synti
#       waveedit
#       widgets
#       al
#       awl
#       ctrl
      
      midiedit ctrl 
      liste mixer driver marker 
      master waveedit mplugins arranger 
      cliplist instruments widgets 
      synti al      
      
      ${QT_LIBRARIES}
      QtSvg
      ${ALSA_LIB}
      ${JACK_LIB}
      ${SNDFILE_LIB}
      ${SAMPLERATE_LIB}
      ${UUID_LIB}
      ${PYLIBS}
      ${LASH_LIB}
      ${FST_LIB}
      ${LIBLO_LIB}
      )

install_targets ( /bin muse )

