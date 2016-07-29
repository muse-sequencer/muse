#=============================================================================
#  MusE
#  Linux Music Editor
#
#  lib_audio_convert/CMakeLists.txt
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

include(${PROJECT_SOURCE_DIR}/pch.txt)

##
## List of source files to compile
##
file (GLOB audio_convert_source_files
      audioconvert.cpp
      )

##
## Define target
##
add_library(audio_convert ${MODULES_BUILD}
      ${PROJECT_BINARY_DIR}/all-pic.h
      ${audio_convert_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${audio_convert_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( audio_convert
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
      OUTPUT_NAME muse_audio_convert
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS audio_convert
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
