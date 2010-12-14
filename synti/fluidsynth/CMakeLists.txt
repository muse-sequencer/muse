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

file (GLOB UI_FILES fluidsynthguibase.ui )
QT4_WRAP_UI ( fluidsynth_uis ${UI_FILES} )

QT4_ADD_RESOURCES (fluidsynth_qrc_files
                  fluidsynth.qrc
                  )

file (GLOB SOURCE_FILES
      fluidsynti.cpp 
      fluidsynthgui.cpp
      fluidsynthgui.h
      )

add_library ( fluidsynth SHARED 
      ${SOURCE_FILES}
      ${fluidsynth_mocs}
      ${fluidsynth_uis}
      ${fluidsynth_qrc_files}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

# target_link_libraries( fluidsynth synti )

# - tell cmake to name target fluidsynth.so instead of 
#   libfluidsynth.so
# - use precompiled header files
#
set_target_properties ( fluidsynth 
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-fvisibility=hidden -include ${PROJECT_BINARY_DIR}/all-pic.h"
   LINK_FLAGS "${FLUIDSYN_LDFLAGS}"   # "-lfluidsynth"
   )

target_link_libraries(fluidsynth
      synti
      ${QT_LIBRARIES}
      # Can't do this here, since FLUIDSYN_LIBRARIES evaluates to 'fluidsynth', the same name as our target.
      # ${FLUIDSYN_LIBRARIES}
      # Try giving full path:  Nope, path is empty!
      #${FLUIDSYN_LIBRARY_DIRS}/${FLUIDSYN_LIBRARIES}
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ fluidsynth )

# FIXME: Won't work
# install_files ( /${MusE_INSTALL_NAME}/synthi TODO README.txt )

install_files ( /${SHARE_INSTALL_PREFIX}/${MusE_INSTALL_NAME}/synthi/fluidsynth TODO )
install_files ( /${SHARE_INSTALL_PREFIX}/${MusE_INSTALL_NAME}/synthi/fluidsynth README.txt )
