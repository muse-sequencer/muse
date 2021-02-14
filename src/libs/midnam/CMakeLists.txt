#=============================================================================
#  MusE
#  Linux Music Editor
#
#  midname/CMakeLists.txt
#  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

file (GLOB midnam_source_files
      midnam.cpp
      )

##
## Define target
##

add_library ( midnam_module SHARED
      ${midnam_source_files}
      )

##
## Compilation flags and target name
##

set_target_properties( midnam_module
      PROPERTIES OUTPUT_NAME muse_midnam_module
      )

target_link_libraries(midnam_module
      xml_module
      midi_controller_module
      mpevent_module
      ${QT_LIBRARIES}
      )

##
## Install location
##

install(TARGETS
        midnam_module
      DESTINATION ${MusE_MODULES_DIR}
      )
