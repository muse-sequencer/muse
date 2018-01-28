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

QT5_WRAP_CPP ( simpler_plugingui_mocs
      simpler_plugingui.h
      )

##
## UI files
##
file (GLOB simpler_plugingui_ui_files
      simplepluginchooserbase.ui
      )
QT5_WRAP_UI ( simpler_plugingui_uis ${simpler_plugingui_ui_files} )

##
## List of source files to compile
##
file (GLOB simpler_plugingui_source_files
      simpler_plugingui.cpp
      )
file (GLOB simpler_plugin_source_files
      simpler_plugin.cpp
      )

##
## Define target
##
add_library ( simpler_plugingui SHARED   ## ${MODULES_BUILD}
      ${PROJECT_BINARY_DIR}/all-pic.h
      ${simpler_plugingui_source_files}
      ${simpler_plugingui_mocs}
      ${simpler_plugingui_uis}
      )
add_library ( simpler_plugin SHARED   ## ${MODULES_BUILD}
      ${PROJECT_BINARY_DIR}/all-pic.h
      ${simpler_plugin_source_files}
      )
      
##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE} 
      ${simpler_plugingui_source_files}
      ${simpler_plugingui_ui_files}
      ${simpler_plugin_source_files}
      CACHE INTERNAL ""
      )
      
##
## Compilation flags and target name
##
set_target_properties( simpler_plugingui
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
      OUTPUT_NAME muse_simpler_plugingui
      )
set_target_properties( simpler_plugin
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
      OUTPUT_NAME muse_simpler_plugin
      )
      
##
## Linkage
##
target_link_libraries(simpler_plugingui
      ${QT_LIBRARIES}
      simpler_plugin
      )
      
target_link_libraries(simpler_plugin
      ${QT_LIBRARIES}
      dl
      )

##
## Install location
##
# if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS simpler_plugingui simpler_plugin
              DESTINATION ${MusE_MODULES_DIR}
            )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )

