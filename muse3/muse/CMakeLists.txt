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
      widgets
      mplugins
      components
      arranger
      cliplist
      ctrl
      driver
      function_dialogs
      instruments
      liste
      marker
      master
      midiedit
      mixer
      waveedit
      )
if(LV2_SUPPORT)
  if(HAVE_GTK2)
    set ( SubDirs ${SubDirs} lv2Gtk2Support )
  endif(HAVE_GTK2)
endif(LV2_SUPPORT)

if (PYTHON_SUPPORT)
      set ( REM_LIB remote )
      set ( SubDirs ${SubDirs} remote )
endif (PYTHON_SUPPORT)

subdirs (${SubDirs})

##
## Expand Qt macros in source files
##
QT5_WRAP_CPP ( muse_moc_headers
      app.h
      cobject.h
      conf.h
      midieditor.h
      plugin.h
      script_delivery.h
      song.h
      transport.h
      value.h
      steprec.h
      vst_native.h
      )

QT5_WRAP_CPP ( wavepreview_moc_headers
      wavepreview.h
      )

##
## Resource files
##
QT5_ADD_RESOURCES (muse_qrc_files
#		  muse.qrc
		  ../resources.qrc
		  )

file (GLOB core_source_files
      app.cpp
      audio.cpp
      audio_fifo.cpp
      audioprefetch.cpp
      audiotrack.cpp
      cobject.cpp
      conf.cpp
      controlfifo.cpp
      ctrl.cpp
      dialogs.cpp
      dssihost.cpp
      event.cpp
      eventlist.cpp
      event_tag_list.cpp
      exportmidi.cpp
      functions.cpp
      gconfig.cpp
      globals.cpp
      help.cpp
      helper.cpp
      importmidi.cpp
      key.cpp
      keyevent.cpp
      metronome_class.cpp
      midi.cpp
      midictrl.cpp
      mididev.cpp
      midieditor.cpp
      midievent.cpp
      midifile.cpp
      midiport.cpp
      midiseq.cpp
      mtc.cpp
      node.cpp
      operations.cpp
      osc.cpp
      part.cpp
      plugin.cpp
      pluglist.cpp
      pos.cpp
      route.cpp
      script_delivery.cpp
      seqmsg.cpp
      shortcuts.cpp
      sig.cpp
      song.cpp
      songfile.cpp
      songfile_discovery.cpp
      stringparam.cpp
      sync.cpp
      synth.cpp
      tempo.cpp
      thread.cpp
      ticksynth.cpp
      track.cpp
      transport.cpp
      transport_obj.cpp
      undo.cpp
      value.cpp
      vst.cpp
      vst_native.cpp
      wave_helper.cpp
      waveevent.cpp
      wavetrack.cpp
      steprec.cpp
      )
file (GLOB main_source_files
      main.cpp
      )
file (GLOB icons_source_files
      icons.cpp
      )

file (GLOB latency_compensator_source_files
      latency_info.cpp
      latency_compensator.cpp
      )
file (GLOB wavepreview_source_files
      wavepreview.cpp
      )

##
## Define target
##

add_library ( latency_compensator_module SHARED
      ${latency_compensator_source_files}
      )
add_library ( wavepreview_module SHARED
      ${wavepreview_moc_headers}
      ${wavepreview_source_files}
      )

add_library ( core ${MODULES_BUILD}
      ${muse_moc_headers}
      ${core_source_files}
      )
add_executable ( muse
      ${main_source_files}
      ${muse_qrc_files}
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
      cliplist
      components
      driver
      function_dialogs
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
      ${CMAKE_CURRENT_BINARY_DIR}/cliplist
      ${CMAKE_CURRENT_BINARY_DIR}/driver
      ${CMAKE_CURRENT_BINARY_DIR}/instruments
      ${CMAKE_CURRENT_BINARY_DIR}/liste
      ${CMAKE_CURRENT_BINARY_DIR}/midiedit
      ${CMAKE_CURRENT_BINARY_DIR}/mixer
      ${CMAKE_CURRENT_BINARY_DIR}/mplugins
      ${CMAKE_CURRENT_BINARY_DIR}/remote
      ${CMAKE_CURRENT_BINARY_DIR}/waveedit
      ${CMAKE_CURRENT_BINARY_DIR}/widgets
      ${CMAKE_CURRENT_BINARY_DIR}/components
      ${CMAKE_CURRENT_BINARY_DIR}/function_dialogs
      )

##
## Compilation flags and target name
##
set_target_properties( core
      PROPERTIES COMPILE_FLAGS "${VST_SDK_QUIRK}"
      OUTPUT_NAME muse_core
      )
set_target_properties( muse
      PROPERTIES OUTPUT_NAME ${MusE_EXEC_NAME}
      )
set_target_properties( icons
      PROPERTIES OUTPUT_NAME muse_icons
      )

set_target_properties( latency_compensator_module
      PROPERTIES OUTPUT_NAME muse_latency_compensator_module
      )
set_target_properties( wavepreview_module
      PROPERTIES OUTPUT_NAME muse_wavepreview_module
      )
      
##
## Linkage
##

target_link_libraries(wavepreview_module
      ${QT_LIBRARIES}
      ${SNDFILE_LIBRARIES}
      ${SAMPLERATE_LIBRARIES}
      )

      
target_link_libraries(core
      al
      arranger
      audio_converter_plugin
      cliplist
      ctrl
      driver
      evdata_module
      icons
      instruments
      latency_compensator_module
      liste
      marker
      master
      memory_module
      midiedit
      mixer
      mpevent_module
      mplugins
      muse_string
      plugin_scan_module
      plugin_cache_writer_module
      Threads::Threads
      synti
      sysex_helper_module
      time_stretch_module
      wave_module
      waveedit
      wavepreview_module
      widgets
      components
      widgets_functiondialogs
      xml_module

      ${QT_LIBRARIES}
      ## Bring in the dssi alsa compatibility libraries if required
      ${DSSI_ALSA_COMPAT_LIBRARIES}
      ${SNDFILE_LIBRARIES}
      # Note that this also appears in the audio_convert/src_resampler shared library.
      ${SAMPLERATE_LIBRARIES}
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

# Certain warnings in lv2 headers are causing errors with recent gcc.
# It seems there's no way to suppress -Werror from within code.
# The slightest warning in some header file can now trigger an error.
# Gcc help warns that only some warnings can be suppressed.
# 'pragma GCC diagnostic ignored "-Wparentheses"' didn't work.
# Looks like no choice but to make the lv2host a module with separate flags.
#     
if(LV2_SUPPORT)
      ##
      ## Expand Qt macros in source files
      ##
      QT5_WRAP_CPP ( lv2host_moc_headers
            lv2host.h
            )
      file (GLOB lv2host_source_files
            lv2host.cpp
            )
      add_library ( lv2host_module ${MODULES_BUILD}
            ${lv2host_moc_headers}
            ${lv2host_source_files}
            )
      set_target_properties( lv2host_module
            # Avoid treating warnings as errors here, unlike the rest of the app.
            PROPERTIES COMPILE_FLAGS "-Wno-error"
            OUTPUT_NAME muse_lv2host_module
            )
            
      target_link_libraries(lv2host_module ${LILV_LIBRARIES})
      target_link_libraries(lv2host_module ${SORD_LIBRARIES})
      target_link_libraries(lv2host_module ${LV2_LIBRARIES})
      
      if(HAVE_GTK2)
            target_link_libraries(lv2host_module lv2_gtk2_support)
      endif(HAVE_GTK2)

      target_link_libraries(lv2host_module ${QT_LIBRARIES})
      
      target_link_libraries(core lv2host_module)
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
      
install(TARGETS
        latency_compensator_module
        wavepreview_module
      DESTINATION ${MusE_MODULES_DIR}
      )
      
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS core icons 
              # latency_compensator_module
              lv2host_module 
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )

