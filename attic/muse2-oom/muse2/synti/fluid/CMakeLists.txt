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

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP ( fluid_mocs
      fluidgui.h
      )

##
## UI files
##
file (GLOB fluid_ui_files
      fluidguibase.ui
      )
QT4_WRAP_UI ( fluid_uis ${fluid_ui_files} )

##
## List of source files to compile
##
file (GLOB fluid_source_files
      fluid.cpp 
      fluidgui.cpp
      )

##
## Define target
##
add_library ( fluid SHARED 
      ${fluid_source_files}
      ${fluid_mocs}
      ${fluid_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${fluid_source_files}
      ${fluid_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target fluid.so instead of 
#   libfluid.so
# - use precompiled header files
#
set_target_properties ( fluid
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      LINK_FLAGS "${FLUIDSYN_LDFLAGS}"   # "-lfluidsynth"
      )

##
## Linkage
##
target_link_libraries(fluid
      synti
      ${QT_LIBRARIES}
      # Can't do this. FLUIDSYN_LIBRARIES = fluidsynth, which is the name 
      # of our fluidsynth.so shared synthesizer library.
      # This caused ../fluidsynth/fluidsynth.so to appear in the link line.
      # ${FLUIDSYN_LIBRARIES}
      # Try giving full path: Nope, path is empty!
      # ${FLUIDSYN_LIBRARY_DIRS}/${FLUIDSYN_LIBRARIES}
      )

##
## Install location
##
install( TARGETS fluid 
      DESTINATION ${MusE_SYNTHI_DIR}
      )
