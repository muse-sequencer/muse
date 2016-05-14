#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

include(${PROJECT_SOURCE_DIR}/pch.txt)

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

set (SubDirs
      arranger
      cliplist
      ctrl
      driver
      instruments
      liste
      marker
      master
      midiedit
      mixer
      mplugins
      waveedit
      widgets
      lv2Gtk2Helper
      )
if(LV2_SUPPORT AND LV2_USE_SUPPLIED_VERSION)
      set ( SubDirs ${SubDirs} lv2Support )
endif(LV2_SUPPORT AND LV2_USE_SUPPLIED_VERSION)

if (PYTHON_SUPPORT)
      set ( REM_CPP_FLAGS "-DENABLE_PYTHON" )
      set ( REM_LIB remote )
      set ( SubDirs ${SubDirs} remote )
endif (PYTHON_SUPPORT)

subdirs (${SubDirs})

##
## Expand Qt macros in source files
##
QT5_WRAP_CPP ( muse_moc_headers
      app.h
      appearance.h
      cobject.h
      conf.h
      confmport.h
      midieditor.h
      miditransform.h
      plugin.h
      song.h
      transport.h
      trackdrummapupdater.h
      value.h
      steprec.h
      lv2host.h
      vst_native.h
      wavepreview.h
      )

##
## Resource files
##
QT5_ADD_RESOURCES (muse_qrc_files
		  muse.qrc
		  )

file (GLOB core_source_files
      app.cpp
      appearance.cpp
      audio.cpp
      audioconvert.cpp
      audioprefetch.cpp
      audiotrack.cpp
      cobject.cpp
      conf.cpp
      confmport.cpp
      controlfifo.cpp
      ctrl.cpp
      dialogs.cpp
      dssihost.cpp
      lv2host.cpp
      event.cpp
      eventlist.cpp
      exportmidi.cpp
      functions.cpp
      gconfig.cpp
      globals.cpp
      help.cpp
      helper.cpp
      importmidi.cpp
      key.cpp
      keyevent.cpp
      memory.cpp
      midi.cpp
      midictrl.cpp
      mididev.cpp
      midieditor.cpp
      midievent.cpp
      midifile.cpp
      midiport.cpp
      midiseq.cpp
      miditransform.cpp
      mpevent.cpp
      mtc.cpp
      node.cpp
      operations.cpp
      osc.cpp
      part.cpp
      plugin.cpp
      pos.cpp
      route.cpp
      seqmsg.cpp
      shortcuts.cpp
      sig.cpp
      song.cpp
      songfile.cpp
      stringparam.cpp
      structure.cpp
      sync.cpp
      synth.cpp
      tempo.cpp
      thread.cpp
      ticksynth.cpp
      track.cpp
      trackdrummapupdater.cpp
      transport.cpp
      undo.cpp
      value.cpp
      vst.cpp
      vst_native.cpp
      wave.cpp
      waveevent.cpp
      wavetrack.cpp
      xml.cpp
      steprec.cpp
      wavepreview.cpp
      )
file (GLOB main_source_files
      main.cpp
      )
file (GLOB icons_source_files
      icons.cpp
      )

##
## Define target
##
add_library ( core ${MODULES_BUILD}
      ${muse_qrc_files}
      ${muse_moc_headers}
      ${core_source_files}
      )
add_executable ( muse
      ${main_source_files}
      )
add_library ( icons ${MODULES_BUILD}
      ${icons_source_files}
      )

set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${main_source_files}
      ${core_source_files}
      CACHE INTERNAL ""
      )

##
## Directories that will be passed to the compiler with -I flag
##
include_directories (
      arranger
      driver
      instruments
      liste
      midiedit
      mixer
      mplugins
      remote
      waveedit
      widgets
      lv2Support
      )

# Make sure to include the uic generated headers paths.
include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/arranger
      ${CMAKE_CURRENT_BINARY_DIR}/driver
      ${CMAKE_CURRENT_BINARY_DIR}/instruments
      ${CMAKE_CURRENT_BINARY_DIR}/liste
      ${CMAKE_CURRENT_BINARY_DIR}/midiedit
      ${CMAKE_CURRENT_BINARY_DIR}/mixer
      ${CMAKE_CURRENT_BINARY_DIR}/mplugins
      ${CMAKE_CURRENT_BINARY_DIR}/remote
      ${CMAKE_CURRENT_BINARY_DIR}/waveedit
      ${CMAKE_CURRENT_BINARY_DIR}/widgets
      ${CMAKE_CURRENT_BINARY_DIR}/widgets/function_dialogs
      )

##
## Compilation flags and target name
##
set_target_properties( core
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      # PROPERTIES COMPILE_FLAGS "-Imidiedit -Iarranger -Iliste -Iwidgets -Imixer -Idriver -Iwaveedit -Implugins -Iinstruments -Iremote ${REM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h "
      # PROPERTIES COMPILE_FLAGS "${REM_CPP_FLAGS} -DINSTPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DINSTLIBDIR='\"${LIBRARY_OUTPUT_DIRECTORY}\"' -include ${PROJECT_BINARY_DIR}/all.h "
      PROPERTIES COMPILE_FLAGS "${VST_SDK_QUIRK} ${REM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_core
      )
set_target_properties( muse
      PROPERTIES OUTPUT_NAME ${MusE_EXEC_NAME}
      )
set_target_properties( icons
      PROPERTIES OUTPUT_NAME muse_icons
      )

##
## Linkage
##
target_link_libraries(core
      al
      arranger
      awl
      cliplist
      ctrl
      driver
      icons
      instruments
      liste
      marker
      master
      midiedit
      mixer
      mplugins
      muse_string
      synti
      waveedit
      widgets
      widgets_functiondialogs

      ${QT_LIBRARIES}
      ${SNDFILE_LIBRARIES}
      ${SAMPLERATE_LIBRARIES}
      ${UUID_LIBRARIES}
      ${REM_LIB}
      ${FST_LIB}
      dl
      )

if(HAVE_LASH)
      target_link_libraries(core ${LASH_LIBRARIES})
endif(HAVE_LASH)

if(OSC_SUPPORT)
      target_link_libraries(core ${LIBLO_LIBRARIES})
endif(OSC_SUPPORT)

if(LV2_SUPPORT)
      if(LV2_USE_SUPPLIED_VERSION)
            message("Building and linking internal LV2 support libraries (lv2, lilv, sord etc.)")
            message("")
            target_link_libraries(core lv2_support)
            if (PCRE_FOUND)
                    target_link_libraries(core ${PCRE_LIBRARIES})
            endif (PCRE_FOUND)
      else(LV2_USE_SUPPLIED_VERSION)
            message("Linking system LV2 support libraries (lv2, lilv, sord etc.)")
            message("")
            target_link_libraries(core ${LILV_LIBRARIES})
            target_link_libraries(core ${SORD_LIBRARIES})
            target_link_libraries(core ${LV2_LIBRARIES})
      endif(LV2_USE_SUPPLIED_VERSION)
endif(LV2_SUPPORT)

target_link_libraries(muse
      midiedit
      core
      )

target_link_libraries(icons
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS muse
      RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
      )
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS core icons
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
