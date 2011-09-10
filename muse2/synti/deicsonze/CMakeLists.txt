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
QT4_WRAP_CPP ( deicsonze_mocs
      deicsonzegui.h
      )

##
## UI files
##
file (GLOB deicsonze_ui_files
      deicsonzegui.ui
      )
QT4_WRAP_UI ( deicsonze_uis ${deicsonze_ui_files} )

##
## Resource files
##
QT4_ADD_RESOURCES (deicsonze_qrc_files
      deicsonze.qrc
      )

##
## List of source files to compile
##
file (GLOB deicsonze_source_files
      deicsonze.cpp
      deicsonzepreset.cpp
      deicsonzeplugin.cpp
      deicsonzefilter.cpp
      deicsonzegui.cpp
      )

##
## Define target
##
add_library ( deicsonze SHARED
      ${deicsonze_source_files}
      ${deicsonze_mocs}
      ${deicsonze_uis}
      ${deicsonze_qrc_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${deicsonze_source_files}
      ${deicsonze_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target deicsonze.so instead of
#   libdeicsonze.so
# - use precompiled header files
#
set_target_properties ( deicsonze
      PROPERTIES PREFIX ""
      #COMPILE_FLAGS "-O2 -include ${PROJECT_BINARY_DIR}/all-pic.h"
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      )

##
## Linkage
##
target_link_libraries(deicsonze
      al
      awl
      synti
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS deicsonze
      DESTINATION ${MusE_SYNTHI_DIR}
      )
install( FILES SutulaBank.dei
      DESTINATION ${MusE_SHARE_DIR}/presets/deicsonze
      )
install( FILES README TODO
      DESTINATION ${MusE_DOC_DIR}/deicsonze
      )
