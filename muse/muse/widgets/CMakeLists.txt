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

QT4_WRAP_CPP (widget_mocs
      doublelabel.h
      noteinfo.h
      dentry.h
      siglabel.h
      simplebutton.h
      tb1.h
      tools.h
      fontsel.h
      ctrlcombo.h
      filedialog.h
      outportcombo.h splitter.h
      quantcombo.h rastercombo.h
      comboQuant.h bigtime.h
      midisync.h
      mixdowndialog.h 
      comment.h gatetime.h
      velocity.h 
      shortcutconfig.h 
      shortcutcapturedialog.h
      greendotbutton.h recordbutton.h
      )

QT4_WRAP_UI (widget_ui_headers
      comment.ui 
      midisync.ui 
      mixdowndialog.ui
      shortcutcapturedialog.ui 
      shortcutconfig.ui 
      gatetime.ui 
      velocity.ui
      )

# add_custom_command(OUTPUT ui_midisync.h
#      COMMAND ${QT_UIC_EXECUTABLE}
#      ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      )

ADD_FILE_DEPENDENCIES(moc_midisync.cxx ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h)

add_library ( widgets STATIC 
      ${widget_ui_headers}
      ${widget_mocs}

      utils.cpp 
      shortcutconfig.cpp
      shortcutconfig.h
      shortcutcapturedialog.cpp
      midisync.cpp 
      comment.cpp
      mixdowndialog.cpp
      velocity.cpp
      gatetime.cpp
      tools.cpp
      doublelabel.cpp 
      noteinfo.cpp 
      dentry.cpp 
      siglabel.cpp 
      simplebutton.cpp
      tb1.cpp
      fontsel.cpp 
      ctrlcombo.cpp 
      filedialog.cpp
      outportcombo.cpp splitter.cpp
      quantcombo.cpp rastercombo.cpp
      comboQuant.cpp bigtime.cpp 
      )

set_target_properties( widgets
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
   )

