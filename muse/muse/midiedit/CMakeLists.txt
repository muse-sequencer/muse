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
      midicmd.h
      gatetime.h
      velocity.h
      quantconfig.h
      dcanvas.h
      drumedit.h
      ecanvas.h
      midieditor.h
      pianoroll.h
      prcanvas.h
      miditracker.h
      trackpattern.h
      )

QT4_WRAP_UI ( midiedit_ui_headers 
      midicmd.ui
      gatetime.ui
      velocity.ui
      quantconfig.ui
      )

add_library ( midiedit STATIC 
      midicmd.h
      gatetime.h
      ${midiedit_ui_headers}
      ${midiedit_mocs}
      midicmd.cpp
      gatetime.cpp
      velocity.cpp
      quantconfig.cpp
      citem.h
      cmd.h
      drummap.h
      midicmd.h
      citem.cpp
      dcanvas.cpp
      drumedit.cpp
      drummap.cpp
      ecanvas.cpp
      midieditor.cpp
      pianoroll.cpp
      prcanvas.cpp
      miditracker.cpp
      trackpattern.cpp
      )

set_target_properties( midiedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
      )

