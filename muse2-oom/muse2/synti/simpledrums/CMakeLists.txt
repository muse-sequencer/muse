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

QT4_WRAP_CPP ( simpledrums_mocs simpledrumsgui.h ssplugingui.h )
# QT4_WRAP_UI (  simpledrums_uis simpledrumsguibase.ui sspluginchooserbase.ui )
QT4_WRAP_UI3 (  simpledrums_uis simpledrumsguibase.ui sspluginchooserbase.ui )

add_library ( simpledrums SHARED
      simpledrums.cpp
      simpledrumsgui.cpp
      simpledrums.h
      ssplugin.cpp
      ssplugingui.cpp
      ssplugin.h
      common.h
      ${simpledrums_mocs}
      ${simpledrums_uis}
      )

# - tell cmake to name target simpledrums.so instead of
#   libsimpledrums.so
# - use precompiled header files
#
set_target_properties ( simpledrums
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-O6 -fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

target_link_libraries(simpledrums
      synti
#      awl
      ${QT_LIBRARIES}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ simpledrums )

