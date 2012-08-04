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
QT4_WRAP_CPP ( simpledrums_mocs
      simpledrumsgui.h
      ssplugingui.h
      )

##
## UI files
##
file (GLOB simpledrums_ui_files 
      simpledrumsguibase.ui
      sspluginchooserbase.ui
      )
QT4_WRAP_UI ( simpledrums_uis ${simpledrums_ui_files} )

##
## List of source files to compile
##
file (GLOB simpledrums_source_files
      simpledrums.cpp
      simpledrumsgui.cpp
      ssplugin.cpp
      ssplugingui.cpp
      )

##
## Define target
##
add_library ( simpledrums SHARED
      ${simpledrums_source_files}
      ${simpledrums_mocs}
      ${simpledrums_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE} 
      ${simpledrums_source_files}
      ${simpledrums_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target simpledrums.so instead of
#   libsimpledrums.so
# - use precompiled header files
#

# Don't override the default optimization. Can be reconsidered 
# according to Mathias' feedback. - Orcan 2012-05-20
#if ( x${CMAKE_BUILD_TYPE} STREQUAL xrelease )
#      SET(SIMPLEDRUMS_OPTFLAGS "-O6")
#endif ( x${CMAKE_BUILD_TYPE} STREQUAL xrelease )

set_target_properties ( simpledrums
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      )

##
## Linkage
##
target_link_libraries(simpledrums
      synti
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS simpledrums 
      DESTINATION ${MusE_SYNTHI_DIR}
      )
install( FILES COPYING README ReleaseNotes.txt
      DESTINATION ${MusE_DOC_DIR}/simpledrums
      )
