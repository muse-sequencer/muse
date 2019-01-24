#=============================================================================
#  MusE
#  Linux Music Editor
#
#  src_resampler/CMakeLists.txt
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
QT5_WRAP_CPP ( src_converter_mocs
      src_converter.h
      )

##
## UI files
##
file (GLOB src_converter_ui_files
      src_resampler_settings_base.ui
      ) 
QT5_WRAP_UI ( src_converter_uis ${src_converter_ui_files} )

##
## List of source files to compile
##
file (GLOB src_converter_source_files
      src_converter.cpp 
      )

##
## Define target
##
# add_library ( src_converter ${MODULES_BUILD}
# add_library ( src_converter MODULE
add_library ( src_converter SHARED
      ${src_converter_source_files}
      ${src_converter_mocs}
      ${src_converter_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${src_converter_source_files}
      ${src_converter_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# - tell cmake to name target src_converter.so instead of 
#   libsrc_converter.so
# - use precompiled header files
#
set_target_properties ( src_converter
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
      # OUTPUT_NAME muse_src_converter
      )

##
## Linkage
##
target_link_libraries(src_converter
      audio_convert
      # Note that this also appears in the core.
      ${SNDFILE_LIBRARIES}
      ${SAMPLERATE_LIBRARIES}
      ${QT_LIBRARIES}
      )

##
## Install location
##
install( TARGETS src_converter
      DESTINATION ${MusE_PLUGINS_DIR}
      )
# install( FILES ChangeLog COPYING README TODO
#       DESTINATION ${MusE_DOC_DIR}/src_converter
#       )
