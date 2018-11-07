#=============================================================================
#  MusE
#  Linux Music Editor
#
#  evdata/CMakeLists.txt
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

include(${PROJECT_SOURCE_DIR}/pch.txt)

##
## List of source files to compile
##

file (GLOB evdata_source_files
      evdata.cpp
      )

##
## Define target
##

add_library ( evdata_module SHARED
      ${evdata_source_files}
      )

##
## Compilation flags and target name
##

set_target_properties( evdata_module
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_evdata_module
      )

##
## Linkage
##

target_link_libraries(evdata_module
      memory_module
      sysex_helper_module
      )

##
## Install location
##

install(TARGETS
        evdata_module
      DESTINATION ${MusE_MODULES_DIR}
      )

# if ( ${MODULES_BUILD} STREQUAL SHARED )
#       install(TARGETS
#             evdata_module
#             DESTINATION ${MusE_MODULES_DIR}
#             )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )
