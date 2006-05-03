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

project(muse)

include ( ${PROJECT_SOURCE_DIR}/cmake/UsePkgConfig1.cmake )

option ( ENABLE_DSSI             "enable Disposable Soft Synth Interface" ON)
option ( ENABLE_VST              "enable VST/win support"                OFF)
option ( ENABLE_FLUID            "enable fluidsynth softsynth plugins"    ON)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTDESIGNER TRUE)

find_package(Qt4)
include(${QT_USE_FILE})

##
## find doxygen
##    TODO

##
## alsa >= 0.9.0
##

PKGCONFIG (alsa 0.9.0 ALSA_INCDIR ALSA_LIBDIR ALSA_LIB ALSA_CPP )
if (NOT ALSA_INCDIR)
      message(FATAL_ERROR "Fatal error: ALSA >= 1.0.0 required")
endif (NOT ALSA_INCDIR)

##
## find sndfile >= 1.0.0
##

PKGCONFIG (sndfile 1.0.0 SNDFILE_INCDIR SNDFILE_LIBDIR SNDFILE_LIB SNDFILE_CPP )
if (NOT SNDFILE_INCDIR)
      message(FATAL_ERROR "Fatal error: libsndfile >= 1.0.0 required")
endif (NOT SNDFILE_INCDIR)

##
## find libsamplerate >= 0.1.0
##

PKGCONFIG (samplerate 0.1.0 SAMPLERATE_INCDIR SAMPLERATE_LIBDIR SAMPLERATE_LIB SAMPLERATE_CPP )
if (NOT SAMPLERATE_INCDIR)
      message(FATAL_ERROR "Fatal error: libsamplerate >= 0.1.0 required")
endif (NOT SAMPLERATE_INCDIR)

##
## find jack >= 0.98.0
##

PKGCONFIG (jack 0.98.0 JACK_INCDIR JACK_LIBDIR JACK_LIB JACK_CPP )
if (NOT JACK_INCDIR)
      message(FATAL_ERROR "Fatal error: JACK >= 0.98.0 required")
endif (NOT JACK_INCDIR)

##
## check for dssi
##

if ( ENABLE_DSSI )
      PKGCONFIG (dssi 0.9.0 DSSI_INCDIR DSSI_LIBDIR DSSI_LIB DSSI_CPP )
      if (NOT DSSI_INCDIR)
            message("Error: dssi >= 0.9.0 not found, disabling feature")
            set ( ENABLE_DSSI OFF )
      endif (NOT DSSI_INCDIR)
endif ( ENABLE_DSSI )

##
## vst
##
## TODO
##

##
## check for fluidsynth
##

if ( ENABLE_FLUID )
      PKGCONFIG (fluidsynth 1.0.0 FLUID_INCDIR FLUID_LIBDIR FLUID_LIB FLUID_CPP )
      if (NOT FLUID_INCDIR)
            message("Error: fluidsynth >= 0.9.0 not found, disabling feature")
            set ( ENABLE_FLUID OFF )
      endif (NOT FLUID_INCDIR)
endif ( ENABLE_FLUID )

#
# produce config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/config.h.in
      ${PROJECT_BINARY_DIR}/config.h
      )

add_custom_target(all-pic.h
   ALL
   cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic.h
   DEPENDS ${PROJECT_SOURCE_DIR}/all.h
   WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
   )

add_custom_target(all.h.pch
   ALL
   ${CMAKE_CXX_COMPILER} 
     -g -Wextra -fno-exceptions -fno-rtti -Wall
     -I/usr/qt4/include
     -I/usr/qt4/include/QtCore
     -I/usr/qt4/include/QtGui
     -o ${PROJECT_BINARY_DIR}/all.h.pch
     ${PROJECT_SOURCE_DIR}/all.h
   DEPENDS ${PROJECT_SOURCE_DIR}/all.h
   WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
   )

add_custom_target(all-pic.h.pch
   ALL
   ${CMAKE_CXX_COMPILER} 
     -fPIC
     -g -Wextra -fno-exceptions -fno-rtti -Wall
     -I/usr/qt4/include
     -I/usr/qt4/include/QtCore
     -I/usr/qt4/include/QtGui
     -o ${PROJECT_BINARY_DIR}/all-pic.h.pch
     ${PROJECT_BINARY_DIR}/all-pic.h
   DEPENDS ${PROJECT_BINARY_DIR}/all-pic.h
   WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
   )
# add_dependencies(all-pic.h.pch all-pic.h)

set(CMAKE_CXX_FLAGS 
   "-g -Wall -Wextra -Winvalid-pch -fno-exceptions -fno-rtti"
   )

subdirs(al awl grepmidi plugins midiplugins synti muse share)

include_directories(
   . 
   ${CMAKE_CURRENT_BINARY}
   ${PROJECT_SOURCE_DIR} 
   ${PROJECT_SOURCE_DIR}/muse
   ${PROJECT_SOURCE_DIR}/muse/widgets
   ${ALSA_INCDIR}
   ${JACK_INCDIR}
   ${SNDFILE_INCDIR}
   ${SAMPLERATE_INCDIR}
   )
