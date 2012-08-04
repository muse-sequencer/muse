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
      )

##
## Linkage
##
target_link_libraries(fluid
      synti
      ${QT_LIBRARIES}
      ${FLUIDSYN_LIBRARIES}
      )

##
## Install location
##
install( TARGETS fluid 
      DESTINATION ${MusE_SYNTHI_DIR}
      )
