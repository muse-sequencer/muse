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

file (GLOB SOURCE_FILES
       audiodev.h alsamidi.cpp alsamidi.h jack.cpp jackaudio.h 
       dummyaudio.cpp alsatimer.cpp alsatimer.h timerdev.h rtctimer.cpp rtctimer.h 
       jackmidi.cpp jackmidi.h
       )

add_library ( driver SHARED
       # ${PROJECT_BINARY_DIR}/all.h.pch
       ${SOURCE_FILES}
       )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

install(TARGETS driver DESTINATION ${MUSE_MODULES_DIR})

target_link_libraries ( driver
      ${ALSA_LIBRARIES}
      ${JACK_LIBRARIES}
      ${QT_LIBRARIES}
      mplugins
      )



set_target_properties( driver
  PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
             OUTPUT_NAME muse_driver
  )

