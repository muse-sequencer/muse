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

QT4_WRAP_CPP ( metronom_mocs metronomgui.h )
QT4_WRAP_UI (  metronom_uis metronomgui.ui )

add_library ( metronom SHARED 
      metronom.cpp 
      metronomgui.cpp
      metronomgui.h
      ${metronom_mocs}
      ${metronom_uis}
      )
target_link_libraries( metronom
   midiplugin
   ${QT_LIBRARIES}
   )

# - tell cmake to name target metronom.so instead of 
#   libmetronom.so
# - use precompiled header files
#
set_target_properties ( metronom 
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

install_targets ( /lib/${MusE_INSTALL_NAME}/midiplugins/ metronom )

