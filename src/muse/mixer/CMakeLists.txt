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
## Expand Qt macros in source files
##
QT5_WRAP_CPP ( mixer_mocs
      amixer.h
      astrip.h
      auxknob.h
      mstrip.h
      rack.h
      strip.h
      )

##
## List of source files to compile
##
file (GLOB mixer_source_files
      amixer.cpp  
      astrip.cpp
      auxknob.cpp 
      mstrip.cpp
      rack.cpp 
      strip.cpp
      )

##
## Define target
##
add_library ( mixer ${MODULES_BUILD}
      ${mixer_source_files}
      ${mixer_mocs}
      )
      
add_dependencies(mixer components)

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${mixer_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties ( mixer
      PROPERTIES OUTPUT_NAME muse_mixer
      )

##
## Linkage
##
target_link_libraries ( mixer
      ${QT_LIBRARIES}
      widgets
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS mixer
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )

