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

add_subdirectory(lib_audio_convert)
add_subdirectory(src_resampler)
add_subdirectory(zita_resampler)
add_subdirectory(rubberband)


##
## List of source files to compile
##
file (GLOB audio_convert_source_files
      audio_converter_plugin.cpp
      audio_converter_settings_group.cpp
      )

##
## Define target
##
add_library(audio_converter_plugin ${MODULES_BUILD}
      ${audio_convert_source_files}
      )

##
## Linkage
##
target_link_libraries(audio_converter_plugin
      xml_module
      dl
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
# set_target_properties( audio_converter_plugin
#       PROPERTIES
#       ## OUTPUT_NAME muse_audio_converter_plugin
#       )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS audio_converter_plugin
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
