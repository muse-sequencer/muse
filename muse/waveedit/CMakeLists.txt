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
## Expand Qt macros in source files
##
QT4_WRAP_CPP ( waveedit_mocs
      editgain.h
      waveedit.h
      waveview.h
      )

##
## UI files
##
file (GLOB waveedit_ui_files
      editgainbase.ui
      )
QT4_WRAP_UI (waveedit_ui_headers ${waveedit_ui_files} )

##
## List of source files to compile
##
file (GLOB waveedit_source_files
      editgain.cpp
      waveedit.cpp
      waveview.cpp
      )

##
## Define target
##
add_library ( waveedit SHARED
      ${waveedit_source_files}
      ${waveedit_ui_headers}
      ${waveedit_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${waveedit_source_files}
      ${waveedit_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( waveedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
      OUTPUT_NAME muse_waveedit
      )

##
## Linkage
##
target_link_libraries( waveedit
      ${QT_LIBRARIES}
      widgets
      )

##
## Install location
##
install(TARGETS waveedit
      DESTINATION ${MusE_MODULES_DIR}
      )
