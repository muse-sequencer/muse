#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2008 by Werner Schweer and others
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
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.1)
if (COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# set libdir if not use -DCMAKE_INSTALL_LIBDIR
if (NOT CMAKE_INSTALL_LIBDIR)
	SET(CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

# for debugging the make system uncomment next line:
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)

# The MusE version number.
SET(MusE_VERSION_MAJOR 2)
SET(MusE_VERSION_MINOR 0)
SET(MusE_VERSION_PATCH 1)
SET(MusE_VERSION       "2.0.1")
SET(MusE_VERSION_FULL  "2.0.1")
SET(MusE_INSTALL_NAME  "muse-2.0")

include ( ${PROJECT_SOURCE_DIR}/cmake/UsePkgConfig1.cmake )
include ( ${PROJECT_SOURCE_DIR}/cmake/TargetDoc.cmake)
# include ( ${PROJECT_SOURCE_DIR}/cmake/WrapMacros.cmake)

option ( ENABLE_LASH        "enable LASH Audio Session Handler"                              ON)
option ( ENABLE_OSC         "enable Open Sound Control (DSSI also recommended)"              ON)   
option ( ENABLE_DSSI        "enable Disposable Soft Synth Interface (OSC also recommended)"  ON)
option ( ENABLE_VST         "enable VST/win support"                                         OFF)
option ( ENABLE_FLUID       "enable fluidsynth softsynth plugins"                            ON)

##
## Just print a notice if this is OS X
##
if (APPLE)
	message("OS X found.")
else (APPLE)
	if (UNIX)
		message("Unix (probably linux) found")
	endif(UNIX)

endif (APPLE)

##
##  look for Qt3
##

# set(QT_MT_REQUIRED TRUE)
# 
#  find_package(Qt3)
# # IF(EXISTS "${CMAKE_ROOT}/Modules/FindQt3.cmake")
# #      include ( "${CMAKE_ROOT}/Modules/FindQt3.cmake" )
# # else (EXISTS "${CMAKE_ROOT}/Modules/FindQt3.cmake")
# #       message(FATAL_ERROR "Fatal error: cmake module FindQt3 required.\n")
# #endif (EXISTS "${CMAKE_ROOT}/Modules/FindQt3.cmake")
# 
# if (NOT QT_FOUND)
#       message(FATAL_ERROR "Fatal error: QT (version >= 3.2.0) required.\n"
#       # "Cmake tries to detect QT4 by searching for 'qmake' in your PATH\n"
#       # "If you have QT4 installed, make sure qmake is found in your PATH."
#       )
# endif (NOT QT_FOUND)
# 
# set (MUSE_QT_WRAP_CPP QT_WRAP_CPP)
# set (MUSE_QT_WRAP_UI QT_WRAP_UI)

# include(${QT_USE_FILE})

##
##  look for Qt4
##

set(QT_MIN_VERSION "4.3.0")
set(QT_USE_QTXML TRUE)
set(QT_USE_QTDESIGNER TRUE)
set(QT_USE_QTNSPLUGIN TRUE)
set(QT_USE_QTSVG TRUE)

## QT3 Compatibility support required, for now. 
set(QT_USE_QT3SUPPORT TRUE)

find_package(Qt4)

if (NOT QT4_FOUND)
      message(FATAL_ERROR "Fatal error: QT (version >= 4.3.0) required.\n"
      "Cmake tries to detect QT4 by searching for 'qmake' in your PATH\n"
      "If you have QT4 installed, make sure qmake is found in your PATH."
      )
endif (NOT QT4_FOUND)

include(${QT_USE_FILE})

# set (MUSE_QT_WRAP_CPP QT4_WRAP_CPP)
# set (MUSE_QT_WRAP_UI QT4_WRAP_UI)
include ( ${PROJECT_SOURCE_DIR}/cmake/QtWrapMacros.cmake)


##
## find doxygen
##    TODO

##
## alsa >= 0.9.0
##

if (APPLE)
	message("Disabling ALSA support due to OS X build.")
else (APPLE)
PKGCONFIG (alsa 0.9.0 ALSA_INCDIR ALSA_LIBDIR ALSA_LIB ALSA_CPP )
if (NOT ALSA_INCDIR)
      message(FATAL_ERROR "Fatal error: ALSA >= 1.0.0 required")
else (NOT ALSA_INCDIR)
	message("Alsa found.")
endif (NOT ALSA_INCDIR)
endif (APPLE)

##
## find sndfile >= 1.0.0
##

PKGCONFIG (sndfile 1.0.0 SNDFILE_INCDIR SNDFILE_LIBDIR SNDFILE_LIB SNDFILE_CPP )
if (NOT SNDFILE_INCDIR)
      message(FATAL_ERROR "Fatal error: libsndfile >= 1.0.0 required")
else (NOT SNDFILE_INCDIR)
	message("sndfile found.")
endif (NOT SNDFILE_INCDIR)

##
## find libsamplerate >= 0.1.0
##

PKGCONFIG (samplerate 0.1.0 SAMPLERATE_INCDIR SAMPLERATE_LIBDIR SAMPLERATE_LIB SAMPLERATE_CPP )
if (NOT SAMPLERATE_INCDIR)
      message(FATAL_ERROR "Fatal error: libsamplerate >= 0.1.0 required")
else (NOT SAMPLERATE_INCDIR)
	message("samplerate found")
endif (NOT SAMPLERATE_INCDIR)

##
## find libuuid 
##

PKGCONFIG (uuid 0.0.1 UUID_INCDIR UUID_LIBDIR UUID_LIB UUID_CPP )
if (NOT UUID_INCDIR)
      message(FATAL_ERROR "Fatal error: libuuid >= 0.0.1 required")
else (NOT UUID_INCDIR)
        message("uuid lib found")
endif (NOT UUID_INCDIR)

# AC_CHECK_HEADER(uuid/uuid.h, UUID_FOUND="yes", UUID_FOUND="no")
# if test x$UUID_FOUND = xno; then
#   AC_MSG_ERROR([Header file uuid.h, from the uuid-dev (sometimes in e2fsprogs) package, is required])
# fi
# 
# AC_CHECK_LIB(uuid, uuid_generate, UUID_FOUND="yes", UUID_FOUND="no")
# if test x$UUID_FOUND = xno; then
#   AC_MSG_ERROR([libuuid (sometimes in e2fsprogs package) is required])
# fi
# UUID_LIBS="-luuid"
# AC_SUBST(UUID_CFLAGS)
# AC_SUBST(UUID_LIBS)
# 
# ## PKG_CHECK_MODULES(UUID, uuid >= 1.0,
# ##   true,
# ##   AC_MSG_ERROR([need libuuid >= 1.0 from e2fsprogs package])
# ## )

##
## find jack >= 0.103.0
##

PKGCONFIG (jack 0.107.0 JACK_INCDIR JACK_LIBDIR JACK_LIB JACK_CPP )
if (NOT JACK_INCDIR)
      PKGCONFIG (jack 0.103.0 JACK_INCDIR JACK_LIBDIR JACK_LIB JACK_CPP )
      if (NOT JACK_INCDIR)
            message(FATAL_ERROR "Fatal error: JACK >= 0.103 required")
      else (NOT JACK_INCDIR)
	      message("JACK 0.103 found")
            set(JACK103 ON)
      endif (NOT JACK_INCDIR)
else (NOT JACK_INCDIR)
	message("JACK 0.107 found")
      set(JACK107 ON)
endif (NOT JACK_INCDIR)

##
## find LASH 
##

if (ENABLE_LASH)
      PKGCONFIG (lash 0.2 LASH_INCDIR LASH_LIBDIR LASH_LIB LASH_CPP )
      if (NOT LASH_INCDIR)
            message("Error: lash >= 0.2 not found, disabling feature")
      else (NOT LASH_INCDIR)
            message("lash >= 0.2 found.")
            set (HAVE_LASH TRUE)
            message("LASH enabled")
      endif (NOT LASH_INCDIR)
else (ENABLE_LASH)
      message("LASH disabled")
endif (ENABLE_LASH)

# muse_enable_lash="no"
# AC_ARG_ENABLE(lash,
#   [  --enable-lash           enable LASH support],[
#   case "$enableval" in
#     "yes")
#       muse_enable_lash="yes"
#       ;;
#     "no")
#       muse_enable_lash="no"
#       ;;
#     *)
#       AC_MSG_ERROR([must use --enable-lash(=yes/no) or --disable-lash])
#       ;;
#   esac
# ])
# 
# if test "$muse_enable_lash" = "yes"; then
#   PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.2, LASH_FOUND="yes", LASH_FOUND="no")
#   AC_SUBST(LASH_CFLAGS)
#   AC_SUBST(LASH_LIBS)
# 
#   if test "$LASH_FOUND" = "yes"; then
#     AC_DEFINE(HAVE_LASH, 1, [whether or not we are supporting lash])
#     LASH_VERSION=$( pkg-config --modversion lash-1.0 )
#     AC_DEFINE_UNQUOTED(LASH_VERSION, "$LASH_VERSION", [The version of lash we're compiling against])
#   fi
# else
#   AC_MSG_WARN([LASH support is disabled])
#   LASH_FOUND="no"
# fi
# AM_CONDITIONAL(HAVE_LASH, test "$LASH_FOUND" = "yes")


##
## check for liblo >= 0.23
##

if (ENABLE_OSC)
      PKGCONFIG (liblo 0.23 LIBLO_INCDIR LIBLO_LIBDIR LIBLO_LIB LIBLO_CPP )
      if (NOT LIBLO_INCDIR)
            message("Error: liblo >= 0.23 not found, disabling feature")
            set ( ENABLE_OSC OFF )
      else (NOT LIBLO_INCDIR)
            message("liblo >= 0.23 found.")
            set (OSC_SUPPORT TRUE)
            message("OSC enabled")
      endif (NOT LIBLO_INCDIR)
else (ENABLE_OSC)
      message("OSC disabled")
endif (ENABLE_OSC)

##
## check for dssi >= 0.9.0
##

if (ENABLE_DSSI)
      PKGCONFIG (dssi 0.9.0 DSSI_INCDIR DSSI_LIBDIR DSSI_LIB DSSI_CPP )
      if (NOT DSSI_INCDIR)
            message("Error: dssi >= 0.9.0 not found, disabling feature")
            set ( ENABLE_DSSI OFF )
      else (NOT DSSI_INCDIR)
            message("dssi >= 0.9.0 found.")
            set (DSSI_SUPPORT TRUE)
            message("DSSI enabled")
      endif (NOT DSSI_INCDIR)
else (ENABLE_DSSI)
      message("DSSI disabled")
endif (ENABLE_DSSI)

if (ENABLE_VST)
      message("VST support enabled")
      set (VST_SUPPORT TRUE)
else (ENABLE_VST)
      message("VST support disabled")
      set (VST_SUPPORT FALSE)
endif (ENABLE_VST)

##
## TODO
##
## Optimizations
##

SET (USE_SSE false)

##
## check for fluidsynth
##

if ( ENABLE_FLUID )
      PKGCONFIG (fluidsynth 1.0.0 FLUID_INCDIR FLUID_LIBDIR FLUID_LIB FLUID_CPP )
      if (NOT FLUID_INCDIR)
            message("Error: fluidsynth >= 0.9.0 not found, disabling feature")
            set ( ENABLE_FLUID OFF )
      else (NOT FLUID_INCDIR)
	    message("Fluidsynth found")
      endif (NOT FLUID_INCDIR)
endif ( ENABLE_FLUID )

#
# produce config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/config.h.in
      ${PROJECT_BINARY_DIR}/config.h
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic-debug.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic-debug.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

# uninstall custom target
# CONFIGURE_FILE(
#     "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
#     IMMEDIATE @ONLY)
# 
# ADD_CUSTOM_TARGET(uninstall
#     "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

include_directories(
   .
   ${PROJECT_SOURCE_DIR}
   ${PROJECT_SOURCE_DIR}/muse
   ${PROJECT_SOURCE_DIR}/muse/widgets
   ${PROJECT_BINARY_DIR}
   ${PROJECT_BINARY_DIR}/muse
   ${PROJECT_BINARY_DIR}/muse/widgets
   ${PROJECT_BINARY_DIR}/muse/instruments
   ${PROJECT_BINARY_DIR}/muse/ctrl

   ${ALSA_INCDIR}
   ${JACK_INCDIR}
   ${SNDFILE_INCDIR}
   ${SAMPLERATE_INCDIR}
   ${LIBLO_INCDIR}
   ${DSSI_INCDIR}
   )

# include (pch.txt)

#
# If the cmake version includes cpack, use it
#
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

  IF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
      SET(CMAKE_INSTALL_MFC_LIBRARIES 1)
      INCLUDE(InstallRequiredSystemLibraries)
  ENDIF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MusE is a virtual audio studio environment")
  SET(CPACK_PACKAGE_VENDOR "Werner Schweer and others")
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_PACKAGE_VERSION_MAJOR "${MusE_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${MusE_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION_PATCH "${MusE_VERSION_PATCH}")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${MusE_INSTALL_NAME}")

  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  SET(CPACK_STRIP_FILES "bin/muse;bin/grepmidi")
  SET(CPACK_PACKAGE_EXECUTABLES "muse" "MusE" "grepmidi" "grepmidi")
  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_CXX_FLAGS         "-Wl,-rpath,${QT_LIBRARY_DIR} -Wall -Wextra -Winvalid-pch -fno-exceptions")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fomit-frame-pointer -ffast-math -fstrength-reduce")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DQT_DEBUG")

# subdirs(al awl grepmidi plugins midiplugins muse share synti)
# subdirs(al lib grepmidi plugins xpm muse share demos doc synti utils)
subdirs(al lib grepmidi plugins muse share demos synti )
#  TODO        packaging utils)

