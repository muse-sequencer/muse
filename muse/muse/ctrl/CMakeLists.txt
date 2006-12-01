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
      configmidictrl.h 
      ctrldialog.h
      ctrledit.h
      )
QT4_WRAP_UI (ctrl_ui_headers 
      configmidictrl.ui 
      ctrldialog.ui
      )

add_library ( ctrl STATIC 
      configmidictrl.cpp 
      ctrldialog.cpp
      ctrldialog.h
      ctrleditor.cpp
      configmidictrl.h
      ctrleditor.h
      ctrledit.cpp
      ${ctrl_mocs}
      ${ctrl_ui_headers}
      )
set_target_properties( ctrl
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
   )

