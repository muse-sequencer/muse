#=============================================================================
#  MusE
#  Linux Music Editor
#
#  xml/CMakeLists.txt
#  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

file (GLOB xml_source_files
      xml.cpp
      )

##
## Define target
##

add_library ( xml_module SHARED
      ${xml_source_files}
      )

##
## Compilation flags and target name
##

set_target_properties( xml_module
      PROPERTIES OUTPUT_NAME muse_xml_module
      )

##
## Linkage
##

target_link_libraries(xml_module
      ${QT_LIBRARIES}
      )

##
## Install location
##

install(TARGETS
        xml_module
      DESTINATION ${MusE_MODULES_DIR}
      )

# if ( ${MODULES_BUILD} STREQUAL SHARED )
#       install(TARGETS
#               # xml_module
#             DESTINATION ${MusE_MODULES_DIR}
#             )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )
