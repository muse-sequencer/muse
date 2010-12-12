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

include(${PROJECT_SOURCE_DIR}/pch.txt)

QT4_WRAP_CPP ( deicsonze_mocs deicsonzegui.h )

file (GLOB UI_FILES deicsonzegui.ui )
QT4_WRAP_UI ( deicsonze_uis ${UI_FILES} )

file (GLOB SOURCE_FILES
      deicsonze.cpp
      deicsonzepreset.cpp
      deicsonzeplugin.cpp
      deicsonzefilter.cpp
      deicsonzegui.cpp
      deicsonzegui.h
      )

add_library ( deicsonze SHARED
      ${SOURCE_FILES}
      ${PROJECT_BINARY_DIR}/all.h
      ${PROJECT_BINARY_DIR}/all-pic.h.pch
      ${deicsonze_mocs}
      ${deicsonze_uis}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

target_link_libraries( deicsonze synti )

# - tell cmake to name target deicsonze.so instead of
#   libdeicsonze.so
# - use precompiled header files
#
set_target_properties ( deicsonze
   PROPERTIES PREFIX ""
   #COMPILE_FLAGS "-O2 -include ${PROJECT_BINARY_DIR}/all-pic.h"
   COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

target_link_libraries(deicsonze
      al awl synti
      ${QT_LIBRARIES}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ deicsonze )

install_files ( /share/${MusE_INSTALL_NAME}/presets/deicsonze SutulaBank.dei )
