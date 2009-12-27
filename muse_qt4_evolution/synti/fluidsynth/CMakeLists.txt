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

QT4_WRAP_CPP ( fluidsynth_mocs fluidsynthgui.h )
QT4_WRAP_UI (  fluidsynth_uis fluidsynthgui.ui )

add_library ( fluidsynth SHARED 
      fluidsynti.cpp 
      fluidsynthgui.cpp
      fluidsynthgui.h
      ${fluidsynth_mocs}
      ${fluidsynth_uis}
      )

target_link_libraries( fluidsynth synti )

# - tell cmake to name target fluidsynth.so instead of 
#   libfluidsynth.so
# - use precompiled header files
#
set_target_properties ( fluidsynth 
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   LINK_FLAGS "-L${FLUID_LIBDIR} ${FLUID_LIB}"
   )

target_link_libraries(fluidsynth
      synti
      ${QT_LIBRARIES}
      ${FLUID_LIB}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ fluidsynth )

