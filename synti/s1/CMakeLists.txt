#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
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
file (GLOB s1_source_files
      s1.cpp
      )

##
## Define target
##
add_library ( s1 SHARED
      ${s1_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${s1_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
# tell cmake to name target s1.so instead of 
# libs1.so
#
set_target_properties ( s1 
      PROPERTIES PREFIX "" 
      COMPILE_FLAGS "-fvisibility=hidden"
      )

##
## Linkage
##
target_link_libraries(s1
      synti
      )

##
## Install location
##
install( TARGETS s1
      DESTINATION ${MusE_SYNTHI_DIR}
      )

