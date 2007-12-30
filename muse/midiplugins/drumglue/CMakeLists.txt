#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2008 by Werner Schweer and others
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

QT4_WRAP_CPP ( drumglue_mocs drumgluegui.h globalinstrumentview.h outputinstrumentview.h )
QT4_WRAP_UI (  drumglue_uis drumgluegui.ui globalinstrumentview.ui outputinstrumentview.ui )

add_library ( drumglue SHARED 
      drumglue.cpp 
      drumgluegui.cpp
      drumgluegui.h
      globalinstrumentview.cpp
      globalinstrumentview.h
      outputinstrumentview.cpp
      outputinstrumentview.h
      ${drumglue_mocs}
      ${drumglue_uis}
      )

target_link_libraries( drumglue
   midiplugin awl
   ${QT_LIBRARIES}
   )

# - tell cmake to name target name.so instead of 
#   libname.so
# - use precompiled header files
set_target_properties ( drumglue
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/midiplugins/ drumglue )

