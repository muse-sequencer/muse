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

QT4_WRAP_CPP ( mixer_mocs amixer.h strip.h astrip.h mstrip.h meter.h rack.h panknob.h auxknob.h routedialog.h )

file (GLOB UI_FILES routedialogbase.ui )
QT4_WRAP_UI ( mixer_uis ${UI_FILES} )

file (GLOB SOURCE_FILES
      amixer.cpp  
      meter.cpp 
      rack.cpp 
      panknob.cpp 
      auxknob.cpp 
      routedialog.cpp 
      strip.cpp astrip.cpp mstrip.cpp
      )

add_library ( mixer SHARED
      ${SOURCE_FILES}
      ${mixer_mocs}
      ${mixer_uis}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

target_link_libraries ( mixer
      ${QT_LIBRARIES}
      widgets
      )

install(TARGETS mixer DESTINATION ${MUSE_MODULES_DIR})

set_target_properties ( mixer
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
              OUTPUT_NAME muse_mixer
   )

