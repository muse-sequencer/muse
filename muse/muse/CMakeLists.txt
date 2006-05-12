#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
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

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

MACRO(WRAP_CPP outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.cpp)
      ENDFOREACH(it)
   ENDMACRO(WRAP_CPP)

MACRO(WRAP_H outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.h)
      ENDFOREACH(it)
   ENDMACRO(WRAP_H)

MACRO(WRAP_UI outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.ui)
      ENDFOREACH(it)
   ENDMACRO(WRAP_UI)

subdirs(
      ctrl widgets master driver instruments marker liste
      )

set (muse_uics
      muse preferences exportmididialog configmidifile
      transport aboutbox transpose
      projectdialog
      templatedialog
      midiedit/quantconfig
      mixer/routedialog
      arranger/configtrack
      )

set(muse_mocs
	projectdialog templatedialog preferences plugingui midiplugin muse 
      song transport conf editor
	cobject transpose track midisynti midiport
      miditrack wavetrack audiotrack audioaux audiooutput
      audioinput audiogroup exportmidi
      midiedit/pianoroll 
      midiedit/prcanvas 
      midiedit/quantconfig 
      midiedit/drumedit
      midiedit/dcanvas 
      midiedit/ecanvas 
      midiedit/ctrledit
      midiedit/midieditor
      mixer/mixer 
      mixer/rack 
      mixer/midirack 
      mixer/routedialog 
      mixer/strip 
      mixer/astrip 
      mixer/mstrip
      arranger/arranger 
      arranger/canvas 
      arranger/configtrack 
      arranger/partdrag 
      arranger/tllineedit
      arranger/tlswidget 
      arranger/tlwidget 
      arranger/tlwlayout
      arranger/trackdrag 
      arranger/trackinfo
      waveedit/waveedit 
      waveedit/waveview
      )

WRAP_H(muse_mocH ${muse_mocs})
WRAP_CPP(muse_mocCPP ${muse_mocs})
WRAP_UI(muse_uiUI ${muse_uics})

QT4_WRAP_CPP (muse_moc_headers ${muse_mocH})
QT4_WRAP_UI (ui_headers ${muse_uiUI})
QT4_ADD_RESOURCES (qrc_files muse.qrc)

add_executable ( muse
      ${qrc_files}
      ticksynth.cpp
      vst.cpp dssihost.cpp synth.cpp plugin.cpp
	mtc.cpp thread.cpp audio.cpp
	audioprefetch.cpp audiowriteback.cpp globals.cpp sync.cpp
	part.cpp undo.cpp songfile.cpp midi.cpp
      importmidi.cpp midifile.cpp icons.cpp event.cpp 
      eventlist.cpp midievent.cpp waveevent.cpp mpevent.cpp
	wave.cpp seqmsg.cpp
	help.cpp
	midictrl.cpp fifo.cpp
	midiseq.cpp helper.cpp route.cpp shortcuts.cpp ctrl.cpp
      gconfig.cpp midirc.cpp midififo.cpp

      midiedit/drummap.cpp 
      midiedit/citem.cpp 

      ${muse_mocCPP}
      ${muse_moc_headers}
      )

set_target_properties( muse
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      )

target_link_libraries(muse
      widgets  ctrl master driver instruments marker liste
      al awl synti
      ${QT_LIBRARIES}
      ${ALSA_LIB}
      ${JACK_LIB}
      ${SNDFILE_LIB}
      )

install_targets ( /bin muse )

