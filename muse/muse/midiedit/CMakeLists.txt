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

QT4_WRAP_CPP ( midiedit_mocs
      gatetime.h
      velocity.h
      quantconfig.h
      ctrledit.h
      dcanvas.h
      drumedit.h
      ecanvas.h
      midieditor.h
      pianoroll.h
      prcanvas.h
      )

QT4_WRAP_UI ( midiedit_ui_headers 
      gatetime.ui
      velocity.ui
      quantconfig.ui
      )

add_library ( midiedit STATIC 
      gatetime.cpp
      velocity.cpp
      quantconfig.cpp
      citem.h
      cmd.h
      drummap.h
      midicmd.h
      citem.cpp
      ctrledit.cpp
      dcanvas.cpp
      drumedit.cpp
      drummap.cpp
      ecanvas.cpp
      midicmd.cpp
      midieditor.cpp
      pianoroll.cpp
      prcanvas.cpp
      ${midiedit_mocs}
      ${midiedit_ui_headers}
      )

set_target_properties( midiedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
      )

