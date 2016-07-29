#=============================================================================
#  MusE
#  Linux Music Editor
#
#  audio_convert/CMakeLists.txt
#  (C) Copyright 2016 Tim E. Real (terminator356 A T sourceforge D O T net)
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
QT5_WRAP_CPP ( rubberband_converter_mocs
      rubberband_converter.h
      )

##
## UI files
##
file (GLOB rubberband_converter_ui_files
      rubberband_settings_base.ui
      ) 
QT5_WRAP_UI ( rubberband_converter_uis ${rubberband_converter_ui_files} )

##
## List of source files to compile
##
file (GLOB rubberband_converter_source_files
      rubberband_converter.cpp 
      )

##
## Define target
##
# add_library ( rubberband_converter ${MODULES_BUILD}
# add_library ( rubberband_converter MODULE
add_library ( rubberband_converter SHARED
      ${rubberband_converter_source_files}
      ${rubberband_converter_mocs}
      ${rubberband_converter_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${rubberband_converter_source_files}
      ${rubberband_converter_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target rubberband_converter.so instead of 
#   librubberband_converter.so
# - use precompiled header files
#
set_target_properties ( rubberband_converter
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      # OUTPUT_NAME muse_rubberband_converter
      )

##
## Linkage
##
target_link_libraries(rubberband_converter
      audio_convert
      ${RUBBERBAND_LIBRARIES}
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS rubberband_converter
      DESTINATION ${MusE_PLUGINS_DIR}
      )
# install( FILES ChangeLog COPYING README TODO
#       DESTINATION ${MusE_DOC_DIR}/rubberband_converter
#       )
