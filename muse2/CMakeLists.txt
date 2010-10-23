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

include(FindPkgConfig)
include(cmake/Summary.cmake)
include(cmake/TargetDoc.cmake)

project(muse)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.1)
if (COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# set libdir if not use -DCMAKE_INSTALL_LIBDIR
if (NOT CMAKE_INSTALL_LIBDIR)
	SET(CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
        SET(SHARE_INSTALL_PREFIX share)
ENDIF(NOT DEFINED SHARE_INSTALL_PREFIX)

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
PKG_CHECK_MODULES(ALSA REQUIRED alsa>=0.9.0)
include_directories(${ALSA_INCLUDE_DIRS})
endif (APPLE)

##
## find sndfile >= 1.0.0
##

PKG_CHECK_MODULES(SNDFILE REQUIRED sndfile>=1.0.0)
include_directories(${SNDFILE_INCLUDE_DIRS})

##
## find libsamplerate >= 0.1.0
##

PKG_CHECK_MODULES(SAMPLERATE REQUIRED samplerate>=0.1.0)
include_directories(${SAMPLERATE_INCLUDE_DIRS})

##
## find libuuid 
##

PKG_CHECK_MODULES(UUID REQUIRED uuid>=0.0.1)
include_directories(${UUID_INCLUDE_DIRS})

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

PKG_CHECK_MODULES(JACK REQUIRED jack>=0.103)
include_directories(${JACK_INCLUDE_DIRS})

##
## find LASH 
##

if (ENABLE_LASH)
      PKG_CHECK_MODULES(LASH lash-1.0>=0.2)
      if (LASH_FOUND)
            include_directories(${LASH_INCLUDE_DIRS})
            set(HAVE_LASH ON)
      endif (LASH_FOUND)           
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
      PKG_CHECK_MODULES(LIBLO liblo>=0.23)
      if (LIBLO_FOUND)
            include_directories(${LIBLO_INCLUDE_DIRS})
            set(OSC_SUPPORT ON)
      endif (LIBLO_FOUND)
else (ENABLE_OSC)
      message("OSC disabled")
endif (ENABLE_OSC)

##
## check for dssi >= 0.9.0
##

if (ENABLE_DSSI)
      PKG_CHECK_MODULES(DSSI dssi>=0.9.0)
      if (DSSI_FOUND)
            include_directories(${DSSI_INCLUDE_DIRS})
            set(DSSI_SUPPORT ON)
      endif (DSSI_FOUND)
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

if (ENABLE_LASH)
      PKG_CHECK_MODULES(LASH lash-1.0>=0.2)
      if (LASH_FOUND)
            include_directories(${LASH_INCLUDE_DIRS})
            set(HAVE_LASH ON)
      endif (LASH_FOUND)           
else (ENABLE_LASH)
      message("LASH disabled")
endif (ENABLE_LASH)


if ( ENABLE_FLUID )
      PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=0.9.0)
      if (FLUIDSYNTH_FOUND)
            include_directories(${FLUIDSYNTH_INCLUDE_DIRS})
            set(HAVE_FLUIDSYNTH ON)
      endif (FLUIDSYNTH_FOUND)
else ( ENABLE_FLUID )
      message("Fluidsynth disabled")
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

set(CMAKE_CXX_FLAGS         "-Wl,-rpath,${QT_LIBRARY_DIR} -Wall -Wextra -Winvalid-pch -fno-exceptions ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fomit-frame-pointer -ffast-math -fstrength-reduce ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DQT_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

# subdirs(al awl grepmidi plugins midiplugins muse share synti)
# subdirs(al lib grepmidi plugins xpm muse share demos doc synti utils)
#subdirs(al lib grepmidi plugins muse share demos synti )
subdirs(al grepmidi plugins muse share synti packaging utils demos)

# Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

# Show a summary of what we got
summary_add("Lash support" HAVE_LASH)
summary_add("OSC (Liblo) support" OSC_SUPPORT)
summary_add("DSSI support" DSSI_SUPPORT)
summary_add("VST support" VST_SUPPORT)
summary_add("Fluidsynth support" HAVE_FLUIDSYNTH)
summary_show()
