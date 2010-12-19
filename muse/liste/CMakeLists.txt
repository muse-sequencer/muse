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

QT4_WRAP_CPP ( liste_mocs
#       listedit.h
#       ctrllistedit.h
#       tracklistedit.h
#       partlistedit.h
#       ieventdialog.h
      
      listedit.h 
      editevent.h
      )

file (GLOB UI_FILES
      editctrlbase.ui
      )

QT4_WRAP_UI ( UIC
      ${UI_FILES}
      )

file (GLOB SOURCE_FILES
#       listedit.cpp listedit.h
#       ctrllistedit.cpp ctrllistedit.h
#       partlistedit.cpp partlistedit.h
#       tracklistedit.cpp tracklistedit.h
#       ieventdialog.cpp ieventdialog.h
      listedit.cpp listedit.h 
      editevent.cpp editevent.h 
      )

add_library ( liste SHARED
      ${SOURCE_FILES}
      ${liste_mocs}
      ${liste_ui_headers}
      ${UIC}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

install(TARGETS liste DESTINATION ${MUSE_MODULES_DIR})

target_link_libraries ( liste
      ${QT_LIBRARIES}
      awl
      widgets
      )

add_dependencies(liste widgets)

set_target_properties( liste
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
              OUTPUT_NAME muse_liste
   )


