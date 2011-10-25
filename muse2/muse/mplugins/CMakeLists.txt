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
if ( ENABLE_EXPERIMENTAL )
      QT4_WRAP_UI ( mplugins_experimental_uis rhythmbase.ui)
      set ( experimental_hdrs rhythm.h )
      set ( experimental_srcs
            rhythm.cpp
            ${mplugins_experimental_uis}
            )
endif ( ENABLE_EXPERIMENTAL )
QT4_WRAP_CPP ( mplugins_mocs
      midifilterimpl.h
      midiitransform.h
      mittranspose.h
      mrconfig.h
      ${experimental_hdrs}
      )

##
## UI files
##
file (GLOB mplugins_ui_files
      midifilter.ui
      mrconfigbase.ui
      )
QT4_WRAP_UI ( mplugins_uis ${mplugins_ui_files} )

##
## List of source files to compile
##
file (GLOB mplugins_source_files
      midifilterimpl.cpp 
      midiitransform.cpp 
      mitplugin.cpp
      mittranspose.cpp 
      mrconfig.cpp 
      random.cpp
      )

##
## Define target
##
add_library ( mplugins ${MODULES_BUILD}
      ${mplugins_source_files}
      ${mplugins_mocs}
      ${mplugins_uis}
      ${experimental_srcs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${mplugins_source_files}
      ${mplugins_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target transform.so instead of 
#   libtransform.so
# - use precompiled header files
#
set_target_properties ( mplugins
      # PROPERTIES PREFIX ""
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_mplugins
      )

##
## Linkage
##
target_link_libraries( mplugins
      # midiplugin
      ${QT_LIBRARIES}
      widgets
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS mplugins
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
