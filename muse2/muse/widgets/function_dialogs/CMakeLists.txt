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
QT4_WRAP_CPP (widgets_functiondialogs_mocs
      crescendo.h
      deloverlaps.h
      gatetime.h
      move.h
      quantize.h
      remove.h
      setlen.h
      transpose.h
      legato.h
      velocity.h
      )

##
## UI files
##
file (GLOB widgets_functiondialogs_ui_files
      crescendobase.ui
      deloverlapsbase.ui
      gatetimebase.ui
      movebase.ui
      quantbase.ui
      removebase.ui
      setlenbase.ui
      transposebase.ui
      legatobase.ui
      velocitybase.ui
      )

QT4_WRAP_UI (widgets_functiondialogs_ui_headers ${widgets_functiondialogs_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_functiondialogs_source_files
      crescendo.cpp
      deloverlaps.cpp
      gatetime.cpp
      move.cpp
      quantize.cpp
      remove.cpp
      setlen.cpp
      transpose.cpp
      legato.cpp
      velocity.cpp
      )

##
## Define target
##
add_library ( widgets_functiondialogs ${MODULES_BUILD} 
      ${widgets_functiondialogs_ui_headers}
      ${widgets_functiondialogs_mocs}
      ${widgets_functiondialogs_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${widgets_functiondialogs_source_files}
      ${widgets_functiondialogs_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( widgets_functiondialogs
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti"
      OUTPUT_NAME muse_widgets_functiondialogs
      )

##
## Linkage
##
target_link_libraries ( widgets_functiondialogs
      ${QT_LIBRARIES}
      icons
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS widgets_functiondialogs
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
