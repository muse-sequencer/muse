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
QT4_WRAP_CPP ( organ_mocs
      organgui.h
      )

##
## UI files
##
file (GLOB organ_ui_files
      organguibase.ui
      )
QT4_WRAP_UI ( organ_uis ${organ_ui_files} )

##
## List of source files to compile
##
file (GLOB organ_source_files
      organ.cpp
      organgui.cpp
      # reverb.cpp
      # routing.cpp
      )

##
## Define target
##
add_library ( organ SHARED
      ${PROJECT_SOURCE_DIR}/muse/xml.cpp
      ${organ_source_files}
      ${organ_mocs}
      ${organ_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${organ_source_files}
      ${organ_ui_files}
      CACHE INTERNAL ""
      )

# - tell cmake to name target organ.so instead of
#   liborgan.so
# - use precompiled header files
#
set_target_properties ( organ
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      )

##
## Linkage
##
target_link_libraries(organ
      synti
      # awl
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS organ
      DESTINATION ${MusE_SYNTHI_DIR}
      )

