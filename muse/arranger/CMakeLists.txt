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

#
# Expand Qt macros
#
QT4_WRAP_CPP (arranger_mocs
      alayout.h
      arranger.h 
      pcanvas.h 
      tlist.h 
      )

#
# List of source files to compile
#
file (GLOB arranger_source_files
      alayout.cpp
      arranger.cpp
      pcanvas.cpp
      tlist.cpp
      )

#
# Define target
#
add_library ( arranger ${MODULES_BUILD}
      ${arranger_source_files}
      ${arranger_mocs}
      )

#
# Append to the list of translations
#
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${arranger_source_files}
      CACHE INTERNAL ""
      )

#
# Compilation flags and target name
#
set_target_properties( arranger
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_arranger
      )

#
# Linkage
#
target_link_libraries ( arranger
      ${QT_LIBRARIES}
      awl
      mixer
      widgets
      )

#
# Install location
#
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS arranger
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
