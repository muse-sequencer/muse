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

QT4_WRAP_CPP ( vam_mocs vamgui.h )
QT4_WRAP_UI (  vam_uis vamgui.ui )

add_library ( vam SHARED 
      vam.cpp 
      vamgui.cpp
      vamgui.h
      ${vam_mocs}
      ${vam_uis}
      )

target_link_libraries( vam synti )

# - tell cmake to name target vam.so instead of 
#   libvam.so
# - use precompiled header files
#
set_target_properties ( vam 
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

target_link_libraries(vam
      al
      synti
      ${QT_LIBRARIES}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ vam )

