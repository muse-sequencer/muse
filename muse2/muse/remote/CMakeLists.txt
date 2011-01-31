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

##
## List of source files to compile
##
file (GLOB remote_source_files
      pyapi.cpp
      )

##
## Define target
##
add_library(remote ${MODULES_BUILD}
      ${remote_source_files}
      ${PROJECT_BINARY_DIR}/all.h.pch
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${remote_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_source_files_properties(
      pyapi.cpp
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -include ${PYTHON_INCLUDES} -DENABLE_PYTHON"
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS remote
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
