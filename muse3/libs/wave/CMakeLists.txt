#=============================================================================
#  MusE
#  Linux Music Editor
#
#  CMakeLists.txt
#  (C) Copyright 2020 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
file (GLOB wave_source_files
      wave.cpp
      )

##
## Define target
##
# add_library(muse_time_stretch_module ${MODULES_BUILD}
add_library(wave_module SHARED
      ${wave_source_files}
      )

##
## Linkage
##
target_link_libraries(wave_module
      time_stretch_module
      audio_converter_plugin
      ${SNDFILE_LIBRARIES}
      )
##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${wave_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties ( wave_module
      PROPERTIES OUTPUT_NAME muse_wave_module
      )


##
## Install location
##
install(TARGETS
        wave_module
      DESTINATION ${MusE_MODULES_DIR}
      )

# if ( ${MODULES_BUILD} STREQUAL SHARED )
#       install(TARGETS wave_module
#             DESTINATION ${MusE_MODULES_DIR}
#             )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )
