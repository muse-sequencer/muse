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

QT4_WRAP_CPP ( waveedit_mocs
      waveedit.h
      waveview.h
      editgain.h
      )

file (GLOB UI_FILES
      editgainbase.ui
      )

QT4_WRAP_UI (waveedit_ui_headers
      ${UI_FILES}
      )

file (GLOB SOURCE_FILES
      waveedit.cpp
      waveview.cpp
      editgain.cpp
      )

add_library ( waveedit SHARED
      ${SOURCE_FILES}
      ${waveedit_ui_headers}
      ${waveedit_mocs}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

install(TARGETS waveedit DESTINATION ${MUSE_MODULES_DIR})

target_link_libraries( waveedit
   ${QT_LIBRARIES}
   widgets
   )


set_target_properties( waveedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
                 OUTPUT_NAME muse_waveedit
      )
