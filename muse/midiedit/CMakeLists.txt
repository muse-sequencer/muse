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
QT4_WRAP_CPP ( midiedit_mocs
      # gatetime.h
      # midicmd.h
      # midieditor.h
      # miditracker.h
      # trackpattern.h
      # velocity.h
      dcanvas.h 
      dlist.h 
      drumedit.h 
      ecanvas.h
      piano.h 
      pianoroll.h 
      prcanvas.h 
      scoreedit.h
      )

##
## List of source files to compile
##
file (GLOB midiedit_source_files
      # citem.cpp
      # gatetime.cpp
      # midicmd.cpp
      # midieditor.cpp
      # miditracker.cpp
      # trackpattern.cpp
      # velocity.cpp
      dcanvas.cpp
      dlist.cpp
      drumedit.cpp
      drummap.cpp
      ecanvas.cpp
      piano.cpp
      pianoroll.cpp
      prcanvas.cpp
      scoreedit.cpp
      )

##
## Define target
##
add_library ( midiedit ${MODULES_BUILD}
      ${midiedit_source_files}
      ${midiedit_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${midiedit_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( midiedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_midiedit
      )

##
## Linkage
##
target_link_libraries ( midiedit
      ${QT_LIBRARIES}
      al
      ctrl
      icons
      widgets
      widgets_functiondialogs
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS midiedit
            DESTINATION ${MusE_MODULES_DIR}
            )
else ( ${MODULES_BUILD} STREQUAL SHARED )
      target_link_libraries (midiedit core)
endif ( ${MODULES_BUILD} STREQUAL SHARED )
