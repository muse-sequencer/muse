#=============================================================================
#  MusE
#  Linux Music Editor
#
#  midi_controller/CMakeLists.txt
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

file (GLOB midi_controller_source_files
      midi_controller.cpp
      )

##
## Define target
##

add_library ( midi_controller_module SHARED
      ${midi_controller_source_files}
      )

##
## Compilation flags and target name
##

set_target_properties( midi_controller_module
      PROPERTIES OUTPUT_NAME muse_midi_controller_module
      )

target_link_libraries(midi_controller_module
      xml_module
      ${QT_LIBRARIES}
      )

##
## Install location
##

install(TARGETS
      midi_controller_module
      DESTINATION ${MusE_MODULES_DIR}
      )
