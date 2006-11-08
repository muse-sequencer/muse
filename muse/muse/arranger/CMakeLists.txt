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

QT4_WRAP_CPP ( arranger_mocs
      arranger.h
      canvas.h
      configtrack.h
      partdrag.h
      tllineedit.h
      tlswidget.h
      tlwidget.h
      trackdrag.h
      trackinfo.h
      )

QT4_WRAP_UI ( arranger_ui_headers 
      configtrack.ui
      midiportinfo.ui
      miditrackinfo.ui
      )

add_library ( arranger STATIC 
      ${arranger_ui_headers}
      ${arranger_mocs}
      selectfilter.h
      tlwlayout.h
      trelement.h
      arranger.cpp
      canvas.cpp
      configtrack.cpp
      partdrag.cpp
      tllineedit.cpp
      tlswidget.cpp
      tlwidget.cpp
      tlwlayout.cpp
      trackdrag.cpp
      trackinfo.cpp
      )

set_target_properties( arranger
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
      )

