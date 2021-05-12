#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

##
## List of source files to compile
##
file (GLOB driver_source_files
       audiodev.cpp
       alsamidi.cpp
       alsatimer.cpp
       dummyaudio.cpp
       jack.cpp
       jackmidi.cpp
       posixtimer.cpp
       rtctimer.cpp
       )
if (HAVE_RTAUDIO)
       file (GLOB driver_source_files
       ${driver_source_files}
       rtaudio.cpp
     )
endif (HAVE_RTAUDIO)


##
## Define target
##
add_library ( driver ${MODULES_BUILD}
       ${driver_source_files}
       )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${driver_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( driver
      PROPERTIES OUTPUT_NAME muse_driver
      )

##
## Linkage
##
target_link_libraries ( driver
      ${ALSA_LIBRARIES}
      ${JACK_LDFLAGS}
      ${JACK_LIBRARIES}
      ${QT_LIBRARIES}
      ${RTAUDIO_LIBRARIES}
      mplugins
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS driver
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
