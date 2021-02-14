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

##
## List of source files to compile
##
file (GLOB time_stretch_source_files
      time_stretch.cpp
      )

##
## Define target
##
# add_library(muse_time_stretch_module ${MODULES_BUILD}
add_library(time_stretch_module SHARED
      ${time_stretch_source_files}
      )

##
## Linkage
##
target_link_libraries(time_stretch_module
      xml_module
      )
##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${time_stretch_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties ( time_stretch_module
      PROPERTIES OUTPUT_NAME muse_time_stretch_module
      )


##
## Install location
##
install(TARGETS
        time_stretch_module
      DESTINATION ${MusE_MODULES_DIR}
      )

# if ( ${MODULES_BUILD} STREQUAL SHARED )
#       install(TARGETS time_stretch_module
#             DESTINATION ${MusE_MODULES_DIR}
#             )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )
