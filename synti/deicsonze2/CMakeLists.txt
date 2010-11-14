#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id: CMakeLists.txt,v 1.1.2.1 2009/12/23 16:15:42 a-lin Exp $
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

include(${PROJECT_SOURCE_DIR}/pch.txt)

QT4_WRAP_CPP ( deicsonze2_mocs deicsonze2gui.h )
QT4_WRAP_UI (  deicsonze2_uis deicsonze2gui.ui )

add_library ( deicsonze2 SHARED
      ${PROJECT_BINARY_DIR}/all.h
      ${PROJECT_BINARY_DIR}/all-pic.h.pch
      deicsonze2.cpp
      deicsonze2preset.cpp
      deicsonze2plugin.cpp
      deicsonze2filter.cpp
      deicsonze2gui.cpp
      deicsonze2gui.h

      ${deicsonze2_mocs}
      ${deicsonze2_uis}
      )

target_link_libraries( deicsonze2 synti )

# - tell cmake to name target deicsonze2.so instead of
#   libdeicsonze2.so
# - use precompiled header files
#
set_target_properties ( deicsonze2
   PROPERTIES PREFIX ""
   #COMPILE_FLAGS "-O2 -include ${PROJECT_BINARY_DIR}/all-pic.h"
   COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

target_link_libraries(deicsonze2
      al awl synti
      ${QT_LIBRARIES}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ deicsonze2 )

install_files ( /${SHARE_INSTALL_PREFIX}/${MusE_INSTALL_NAME}/presets/deicsonze2 SutulaBank.dei )
