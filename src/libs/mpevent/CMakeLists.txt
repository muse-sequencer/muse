#=============================================================================
#  MusE
#  Linux Music Editor
#
#  mpevent/CMakeLists.txt
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

file (GLOB mpevent_source_files
      mpevent.cpp
      )

##
## Define target
##

add_library ( mpevent_module SHARED
      ${mpevent_source_files}
      )

##
## Compilation flags and target name
##

set_target_properties( mpevent_module
      PROPERTIES OUTPUT_NAME muse_mpevent_module
      )

##
## Linkage
##

target_link_libraries(mpevent_module
      evdata_module
      sysex_helper_module
      )

##
## Install location
##

install(TARGETS
        mpevent_module
      DESTINATION ${MusE_MODULES_DIR}
      )

# if ( ${MODULES_BUILD} STREQUAL SHARED )
#       install(TARGETS
#             mpevent_module
#             DESTINATION ${MusE_MODULES_DIR}
#             )
# endif ( ${MODULES_BUILD} STREQUAL SHARED )
