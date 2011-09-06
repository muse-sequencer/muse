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
QT4_WRAP_CPP ( liste_mocs
      # listedit.h
      # ctrllistedit.h
      # tracklistedit.h
      # partlistedit.h
      # ieventdialog.h
      editevent.h      
      listedit.h 
      )

##
## UI files
##
file (GLOB liste_ui_files
      editctrlbase.ui
      )
QT4_WRAP_UI ( liste_uis ${liste_ui_files} )

##
## List of source files to compile
##
file (GLOB liste_source_files
      # listedit.cpp
      # ctrllistedit.cpp
      # partlistedit.cpp
      # tracklistedit.cpp
      # ieventdialog.cpp
      editevent.cpp
      listedit.cpp
      )

##
## Define target
##
add_library ( liste ${MODULES_BUILD}
      ${liste_source_files}
      ${liste_mocs}
      ${liste_uis}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${liste_source_files}
      ${liste_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( liste
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_liste
      )

##
## Linkage
##
target_link_libraries ( liste
      ${QT_LIBRARIES}
      awl
      widgets
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS liste
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
