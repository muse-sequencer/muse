#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

include(${PROJECT_SOURCE_DIR}/pch.txt)

##
## List of source files to compile
##
file (GLOB al_source_files
      al.cpp
      dsp.cpp
      sig.cpp
      xml.cpp
      )
if (USE_SSE)
      file (GLOB al_source_files
      ${al_source_files}
      dspSSE.cpp
      dspXMM.cpp
      )
endif (USE_SSE)

##
## Define target
##
add_library(al SHARED
      ${PROJECT_BINARY_DIR}/all.h.pch
      ${al_source_files}
      )

##
## Compilation flags and target name
##
set_target_properties( al
      PROPERTIES OUTPUT_NAME muse_al
      )
set_source_files_properties(
      al.cpp
      dsp.cpp 
      dspXMM.cpp
      sig.cpp
      xml.cpp
      PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
      )
set_source_files_properties(
      dspSSE.cpp
      PROPERTIES COMPILE_FLAGS "-x assembler"
      )

##
## Linkage
##
target_link_libraries ( al
      ${QT_LIBRARIES}
      )

##
## Install location
##
install(TARGETS al 
      DESTINATION ${MusE_MODULES_DIR}
      )
