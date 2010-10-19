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

QT4_WRAP_CPP (arranger_mocs
      arranger.h 
      pcanvas.h 
      tlist.h 
      alayout.h
      )

#QT4_WRAP_UI (arranger_ui_headers
# QT4_WRAP_UI3 (arranger_ui_headers
#       )

add_library ( arranger STATIC
      arranger.cpp arranger.h 
      pcanvas.cpp pcanvas.h 
      tlist.cpp tlist.h 
      alayout.cpp alayout.h 
      trackinfo.cpp trackautomationview.h
      
      ${arranger_mocs}
      # ${arranger_ui_headers}
      )
set_target_properties( arranger
  PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
  )

