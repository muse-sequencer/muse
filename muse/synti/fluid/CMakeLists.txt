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

QT4_WRAP_CPP ( fluid_mocs fluidgui.h )
QT4_WRAP_UI (  fluid_uis fluidgui.ui )

add_library ( fluid SHARED 
      fluid.cpp 
      fluidgui.cpp
      fluidgui.h
      ${fluid_mocs}
      ${fluid_uis}
      )

# - tell cmake to name target fluid.so instead of 
#   libfluid.so
# - use precompiled header files
#
set_target_properties ( fluid
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

install_targets ( /lib/muse-1.0pre1/synthi/ fluid )

