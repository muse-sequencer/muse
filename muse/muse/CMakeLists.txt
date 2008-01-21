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

add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/revision.h
   COMMAND svn info ${PROJECT_SOURCE_DIR} | grep Revision | cut -f 2 -d \" \" > ${CMAKE_BINARY_DIR}/revision.h
   WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
   )

set_source_files_properties(
      ${PROJECT_BINARY_DIR}/revision.h
      PROPERTIES generated true
      )

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

subdirs (
      ctrl
      midiedit
      widgets
      master
      driver
      instruments
      marker
      liste
      mixer
      arranger
      waveedit
      )

QT4_WRAP_UI  (muse_ui_headers
      preferences.ui
      exportmididialog.ui
      configmidifile.ui
      transport.ui
      aboutbox.ui
      transpose.ui
      projectpropsdialog.ui
      projectdialog.ui
      templatedialog.ui
      importmidi.ui
      )

QT4_WRAP_CPP ( muse_moc_headers
      projectpropsdialog.h
	projectdialog.h
      templatedialog.h
      preferences.h
      plugingui.h
      midiplugin.h
      muse.h
      song.h
      transport.h
      conf.h
      editor.h
	cobject.h
      transpose.h
      track.h
      miditrackbase.h
      midisynti.h
      miditrack.h
      wavetrack.h
      audiotrack.h
      audiooutput.h
      audioinput.h
      audiogroup.h
      synth.h
      exportmidi.h
      importmidi.h
      midiinport.h
      midioutport.h
      )

QT4_ADD_RESOURCES (muse_qrc_files muse.qrc)

add_executable ( muse
      ${muse_qrc_files}
      ${muse_moc_headers}
      ${muse_ui_headers}
      ${PROJECT_BINARY_DIR}/all.h.pch

      ticksynth.cpp
      vst.cpp
      dssihost.cpp
      synth.cpp
      plugin.cpp
      ladspaplugin.cpp
      auxplugin.cpp
      pipeline.cpp
	mtc.cpp
      thread.cpp
      audio.cpp
	audioprefetch.cpp
      audiowriteback.cpp
      globals.cpp
      sync.cpp
	part.cpp
      undo.cpp
      songfile.cpp
      midi.cpp
      importmidi.cpp
      importmidi.h
      midifile.cpp
      icons.cpp
      event.cpp
      eventlist.cpp
      midieventbase.cpp
      waveevent.cpp
      midievent.cpp
	wave.cpp
      seqmsg.cpp
	help.cpp
	midictrl.cpp
      fifo.cpp
	midiseq.cpp
      helper.cpp
      route.cpp
      shortcuts.cpp
      ctrl.cpp
      gconfig.cpp
      midirc.cpp
      midififo.cpp
      midioutport.cpp
      midiinport.cpp
      midiout.cpp

      projectpropsdialog.cpp
	projectdialog.cpp
      templatedialog.cpp
      preferences.cpp
      plugingui.cpp
      midiplugin.cpp
      muse.cpp
      song.cpp
      songpart.cpp
      songtrack.cpp
      transport.cpp
      conf.cpp
      editor.cpp
	cobject.cpp
      transpose.cpp
      track.cpp
      miditrackbase.cpp
      midisynti.cpp
      miditrack.cpp
      wavetrack.cpp
      audiotrack.cpp
      audiooutput.cpp
      audioinput.cpp
      audiogroup.cpp
      exportmidi.cpp
      revision.cpp
      ${PROJECT_BINARY_DIR}/revision.h
      )

set_target_properties( muse
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      )

target_link_libraries(muse
      ctrl
      midiedit
      master
      driver
      instruments
      marker
      liste
      synti
      arranger
      mixer
      waveedit
      widgets
      ctrl
      al
      awl
      ${QT_LIBRARIES}
      QtSvg
      ${ALSA_LIB}
      ${JACK_LIB}
      ${SNDFILE_LIB}
      ${SAMPLERATE_LIB}
      ${LIBLO_LIB}
      )

install_targets ( /bin muse )

