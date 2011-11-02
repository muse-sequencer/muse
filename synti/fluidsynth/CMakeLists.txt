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

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP ( fluidsynth_mocs
      fluidsynthgui.h
      )

##
## UI files
##
file (GLOB fluidsynth_ui_files
      fluidsynthguibase.ui
      )
QT4_WRAP_UI ( fluidsynth_uis ${fluidsynth_ui_files} )

##
## Resource files
##
QT4_ADD_RESOURCES (fluidsynth_qrc_files
      fluidsynth.qrc
      )

##
## List of source files to compile
##
file (GLOB fluidsynth_source_files
      fluidsynti.cpp 
      fluidsynthgui.cpp
      )

##
## Define target
##
# Nov 1, 2011. fluidsynth module renamed fluid_synth to avoid bizarre new conflict on Ubuntu 11.10, seems due to LINK_FLAGS line.
# Got undefined symbol fluid_synth_set_reverb at startup at runtime.
add_library ( fluid_synth SHARED 
      ${fluidsynth_source_files}
      ${fluidsynth_mocs}
      ${fluidsynth_uis}
      ${fluidsynth_qrc_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE 
      ${FILES_TO_TRANSLATE}
      ${fluidsynth_source_files}
      ${fluidsynth_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target fluidsynth.so instead of 
#   libfluidsynth.so
# - use precompiled header files
#
set_target_properties ( fluid_synth 
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      # LINK_FLAGS "${FLUIDSYN_LDFLAGS}"   # "-lfluidsynth"
      )

##
## Linkage
##
target_link_libraries(fluid_synth
      synti
      ${QT_LIBRARIES}
      # Can't do this here, since FLUIDSYN_LIBRARIES evaluates 
      # to 'fluidsynth', the same name as our target.
      # Fixed Nov 1, 2011
      ${FLUIDSYN_LIBRARIES}
      )

##
## Install location
##
install( TARGETS fluid_synth 
      DESTINATION ${MusE_SYNTHI_DIR}
      )
install( FILES README.txt TODO
      DESTINATION ${MusE_DOC_DIR}/fluidsynth/
      )
