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
      listedit.h
      ctrllistedit.h
      tracklistedit.h
      partlistedit.h
      ieventdialog.h
      )

QT4_WRAP_UI ( liste_ui_headers
      ctrllistedit.ui
      partlistedit.ui
      tracklistedit.ui
      )

add_library ( liste STATIC
      listedit.cpp listedit.h
      ctrllistedit.cpp ctrllistedit.h
      partlistedit.cpp partlistedit.h
      tracklistedit.cpp tracklistedit.h
      ieventdialog.cpp ieventdialog.h
      ${liste_mocs}
      ${liste_ui_headers}
      )
set_target_properties( liste
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
   )


