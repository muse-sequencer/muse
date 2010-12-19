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

QT4_WRAP_CPP (ctrl_mocs
#       configmidictrl.h
#       definemidictrl.h
#       ctrldialog.h
#       ctrledit.h
        
      ctrledit.h 
      ctrlcanvas.h 
      ctrlpanel.h
      )

file (GLOB SOURCE_FILES
      ctrledit.cpp ctrledit.h 
      ctrlcanvas.cpp ctrlcanvas.h 
      ctrlpanel.cpp ctrlpanel.h
      )

add_library ( ctrl SHARED
#       configmidictrl.cpp
#       definemidictrl.cpp
#       ctrldialog.cpp
#       ctrleditor.cpp
#       ctrledit.cpp
      
      ${SOURCE_FILES}
      ${ctrl_mocs}
      # ${ctrl_ui_headers}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

target_link_libraries ( ctrl
      ${QT_LIBRARIES}
      widgets
      )

install(TARGETS ctrl DESTINATION ${MUSE_MODULES_DIR})

set_target_properties( ctrl
  PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
             OUTPUT_NAME muse_ctrl
  )

