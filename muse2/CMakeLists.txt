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

include(FindPkgConfig)
include(cmake/Summary.cmake)
include(cmake/TargetDoc.cmake)

project(muse)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.1)
if (COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if (NOT DEFINED LIB_SUFFIX)
        SET(LIB_SUFFIX "" CACHE STRING "Suffix for installed library path. Ex. 64 for lib64")
endif (NOT DEFINED LIB_SUFFIX)

# set libdir if not use -DLIB_INSTALL_DIR
if (NOT LIB_INSTALL_DIR)
        if(CMAKE_INSTALL_LIBDIR)
                SET(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
        else(CMAKE_INSTALL_LIBDIR)
                SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
        endif(CMAKE_INSTALL_LIBDIR)
endif (NOT LIB_INSTALL_DIR)

if (LIB_SUFFIX)
        message(" Install libraries to: " ${LIB_INSTALL_DIR} )          
endif (LIB_SUFFIX)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
        SET(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share")
ENDIF(NOT DEFINED SHARE_INSTALL_PREFIX)

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

# If no CMAKE_BUILD_TYPE is given on the command line,
#  cmake either uses the cached value, or 'empty' (plain un-opt build). 
# And yet the only way a user can reset a cached CMAKE_BUILD_TYPE
#  is with "-DCMAKE_BUILD_TYPE=". So we cannot interfere with this.
# We should probably not attempt to do this at all.
# Installation instructions now updated to be more specific, give options.
#endif (NOT DEFINED CMAKE_BUILD_TYPE)
#if (NOT CMAKE_BUILD_TYPE)
#    message("No CMAKE_BUILD_TYPE specified. Setting to Release.")
#     set(CMAKE_BUILD_TYPE release)
#endif (NOT CMAKE_BUILD_TYPE)


# for debugging the make system uncomment next line:
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)

# The MusE version number.
SET(MusE_VERSION_MAJOR 2)
SET(MusE_VERSION_MINOR 0)
SET(MusE_VERSION_PATCH 0)
SET(MusE_VERSION       "2.0")
SET(MusE_VERSION_FULL  "2.0")
SET(MusE_INSTALL_NAME  "muse-2.0")
SET(MusE_EXEC_NAME     "muse2")

## The directory where we will install the shared components:
SET(MusE_MODULES_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/modules)

## Synthesizers directory
SET(MusE_SYNTHI_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/synthi)

## Plugins directory
SET(MusE_PLUGINS_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/plugins)

## Top documentation dir
IF(NOT DEFINED MusE_DOC_DIR)
      SET(MusE_DOC_DIR ${SHARE_INSTALL_PREFIX}/doc/${MusE_INSTALL_NAME}/)
ENDIF(NOT DEFINED MusE_DOC_DIR)

## Data directory
SET(MusE_SHARE_DIR ${SHARE_INSTALL_PREFIX}/${MusE_INSTALL_NAME})

## Lib directory
SET(MusE_LIB_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME})

FIND_PROGRAM(SVNVER svnversion)
if (${SVNVER} STREQUAL "SVNVER-NOTFOUND")
      SET (MusE_SVNVER 0)
else (${SVNVER} STREQUAL "SVNVER-NOTFOUND")
      EXEC_PROGRAM( svnversion
            ARGS ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE MusE_SVNVER )
endif (${SVNVER} STREQUAL "SVNVER-NOTFOUND")

# Do we need this? - Orcan:
# include ( ${PROJECT_SOURCE_DIR}/cmake/WrapMacros.cmake)

option ( ENABLE_LASH        "enable LASH Audio Session Handler (or LADISH compatibility layer)"     ON)
option ( ENABLE_OSC         "enable Lightweight Open Sound Control (liblo) (DSSI also recommended)"  ON)   
option ( ENABLE_DSSI        "enable Disposable Soft Synth Interface (dssi) (OSC also recommended)" ON)
option ( ENABLE_VST         "enable VST/win support (deprecated)"                                OFF)
option ( ENABLE_FLUID       "enable fluidsynth softsynth plugins"                                ON)
option ( ENABLE_EXPERIMENTAL "enable building experimental features"                             OFF)
option ( ENABLE_PYTHON      "enable Python control support"                                      OFF)
option ( UPDATE_TRANSLATIONS "Update source translation share/locale/*.ts files 
         (WARNING: This will modify the .ts files in the source tree!!)"                         OFF)
option ( MODULES_BUILD_STATIC "Build type of internal modules"                                   OFF)

if ( MODULES_BUILD_STATIC )
      SET(MODULES_BUILD STATIC )
else ( MODULES_BUILD_STATIC )
      SET(MODULES_BUILD SHARED )
      SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
      SET(CMAKE_INSTALL_RPATH ${MusE_MODULES_DIR})
endif ( MODULES_BUILD_STATIC )

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
##  look for Qt4
##

set(QT_MIN_VERSION "4.3.0")
set(QT_USE_QTXML TRUE)
set(QT_USE_QTDESIGNER TRUE)
#set(QT_USE_QTNSPLUGIN TRUE)
set(QT_USE_QTSVG TRUE)

find_package(Qt4)

if (NOT QT4_FOUND)
      message(FATAL_ERROR "Fatal error: QT (version >= 4.3.0) required.\n"
      "Cmake tries to detect QT4 by searching for 'qmake' in your PATH\n"
      "If you have QT4 installed, make sure qmake is found in your PATH."
      )
endif (NOT QT4_FOUND)

# Needed for plugins factory:
SET(QT_USE_QTUITOOLS TRUE)

include(${QT_USE_FILE})

##
##   Begin MANDATORY packages...
##


##
## find doxygen program
##

FIND_PROGRAM(DOXY doxygen)

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

##
## find jack >= 0.103.0
##

PKG_CHECK_MODULES(JACK REQUIRED jack>=0.103)
include_directories(${JACK_INCLUDE_DIRS})

##
##   End MANDATORY packages.
##

##
##   Begin OPTIONAL packages...
##

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
## check for python >= 2.4
##

if (ENABLE_PYTHON)
      include(FindPythonLibs)
      if (PYTHONLIBS_FOUND)
# Older cmake does not define PYTHON_INCLUDE_DIRS and PYTHONLIBS_VERSION_STRING.
# We need to define them ourselves to do a version comparison
            if(DEFINED PYTHON_INCLUDE_PATH AND NOT DEFINED PYTHON_INCLUDE_DIRS)
                  set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_PATH})
            endif(DEFINED PYTHON_INCLUDE_PATH AND NOT DEFINED PYTHON_INCLUDE_DIRS)
            if(NOT DEFINED PYTHONLIBS_VERSION_STRING)
                  STRING(REGEX REPLACE ".*python(2\\.[0-9]+)" "\\1"
                         PYTHONLIBS_VERSION_STRING "${PYTHON_INCLUDE_DIRS}")
            endif(NOT DEFINED PYTHONLIBS_VERSION_STRING)
            if (NOT PYTHONLIBS_VERSION_STRING VERSION_LESS 2.4)
                  set(PYTHON_SUPPORT ON)
            endif (NOT PYTHONLIBS_VERSION_STRING VERSION_LESS 2.4)
      endif (PYTHONLIBS_FOUND)
endif (ENABLE_PYTHON)

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
      message("deprecated VST support enabled")
      set (VST_SUPPORT TRUE)
else (ENABLE_VST)
      #message("VST support disabled")
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
      # Be careful with naming here since we have two shared synth libraries
      #  to be built later, named 'fluidsynth' and 'fluid' !
      PKG_CHECK_MODULES(FLUIDSYN fluidsynth>=0.9.0)
      if (FLUIDSYN_FOUND)
            include_directories(${FLUIDSYN_INCLUDE_DIRS})
            set(HAVE_FLUIDSYNTH ON)
            ## This stuff is just F.Y.I. Please keep...
            # message("FLUIDSYN_LIBRARIES " ${FLUIDSYN_LIBRARIES} )          # "fluidsynth"
            # message("FLUIDSYN_LDFLAGS " ${FLUIDSYN_LDFLAGS} )              # "-lfluidsynth"
            # message("FLUIDSYN_LDFLAGS_OTHER " ${FLUIDSYN_LDFLAGS_OTHER} )  # empty
            # message("FLUIDSYN_INCLUDE_DIRS " ${FLUIDSYN_INCLUDE_DIRS} )    # empty
            # message("FLUIDSYN_CFLAGS " ${FLUIDSYN_CFLAGS} )                # empty
            # message("FLUIDSYN_CFLAGS_OTHER " ${FLUIDSYN_CFLAGS_OTHER} )    # empty
      endif (FLUIDSYN_FOUND)
else ( ENABLE_FLUID )
      message("Fluidsynth disabled")
endif ( ENABLE_FLUID )

##
##   End OPTIONAL packages.
##


if ( ENABLE_EXPERIMENTAL )
      set(CMAKE_CXX_FLAGS -DBUILD_EXPERIMENTAL ${CMAKE_CXX_FLAGS})
endif ( ENABLE_EXPERIMENTAL )

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

  # Don't pack the binary tree and the subversion directories
  SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.svn/;${CMAKE_BINARY_DIR}/*")

  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  SET(CPACK_STRIP_FILES "bin/muse;bin/grepmidi")
  SET(CPACK_PACKAGE_EXECUTABLES "muse" "MusE" "grepmidi" "grepmidi")
  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_CXX_FLAGS         "-Wall -Wextra -Winvalid-pch -fno-exceptions -fPIC ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fomit-frame-pointer -ffast-math -fstrength-reduce -fPIC ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DQT_DEBUG -fPIC ${CMAKE_CXX_FLAGS_DEBUG}")

# subdirs(al awl grepmidi plugins midiplugins muse share synti)
# subdirs(al lib grepmidi plugins xpm muse share demos doc synti utils)
#subdirs(al lib grepmidi plugins muse share demos synti )
# NOTE: share/ directory needs to be at the end so that the translations
#       are scanned before coming to share/locale
subdirs(al awl grepmidi man plugins muse synti packaging utils demos share)

## Install doc files
file (GLOB doc_files
      AUTHORS
      ChangeLog
      COPYING
      NEWS
      README
      README.de
      README.effects-rack
      README.instruments
      README.ladspaguis
      README.shortcuts
      README.softsynth
      README.translate
      SECURITY
      )
INSTALL( FILES ${doc_files} DESTINATION ${MusE_DOC_DIR})

## Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

##
## Report errors and warnings and hints  
##

message("\n")

if (${DOXY} STREQUAL "DOXY-NOTFOUND")
      message("** ERROR: The program 'doxygen' is required, but was not found.")
endif (${DOXY} STREQUAL "DOXY-NOTFOUND")
  
if (NOT ALSA_FOUND)
      message("** ERROR: alsa >= 0.9.0 is required, but development files were not found.")
endif (NOT ALSA_FOUND)           

if (NOT SNDFILE_FOUND)
      message("** ERROR: sndfile >= 1.0.0 is required, but development files were not found.")
endif (NOT SNDFILE_FOUND)           

if (NOT SAMPLERATE_FOUND)
      message("** ERROR: samplerate >= 0.1.0 is required, but development files were not found.")
endif (NOT SAMPLERATE_FOUND)           

if (NOT UUID_FOUND)
      message("** ERROR: uuid >= 0.0.1 is required, but development files were not found.")
endif (NOT UUID_FOUND)           

if (NOT JACK_FOUND)
      message("** ERROR: jack >= 0.103 is required, but development files were not found.")
endif (NOT JACK_FOUND)           

if (ENABLE_LASH AND (NOT LASH_FOUND))
        message("** WARNING: lash (>= 0.2) was enabled, but development files were not found. ")
        message("** HINT: Don't have LASH? Try installing the LADISH LASH compatibility package instead.")
endif (ENABLE_LASH AND (NOT LASH_FOUND))

if (ENABLE_OSC AND (NOT LIBLO_FOUND))
        message("** WARNING: liblo (>= 0.23) (Lightweight Open Sound Control) was enabled, but development files were not found.")
endif (ENABLE_OSC AND (NOT LIBLO_FOUND))

if (ENABLE_PYTHON AND (NOT PYTHONLIBS_FOUND))
        message("** WARNING: python was enabled, but development files were not found.")
endif (ENABLE_PYTHON AND (NOT PYTHONLIBS_FOUND))

if (ENABLE_DSSI AND (NOT DSSI_FOUND))
        message("** WARNING: dssi (>= 0.9.0) was enabled, but development files were not found.")
endif (ENABLE_DSSI AND (NOT DSSI_FOUND))

if (ENABLE_FLUID AND (NOT FLUIDSYN_FOUND))
        message("** WARNING: fluidsynth (>= 0.9.0) was enabled, but development files were not found.")
endif (ENABLE_FLUID AND (NOT FLUIDSYN_FOUND))

message("")

## Show a summary of what we got
summary_add("Lash support" HAVE_LASH)
summary_add("OSC (Liblo) support" OSC_SUPPORT)
summary_add("Python support" PYTHON_SUPPORT)
summary_add("DSSI support" DSSI_SUPPORT)
#summary_add("VST support" VST_SUPPORT)
summary_add("Fluidsynth support" HAVE_FLUIDSYNTH)
summary_add("Experimental features" ENABLE_EXPERIMENTAL)
summary_show()

if ( MODULES_BUILD_STATIC )
    message (" Internal modules will be built statically into the final binary.")
else ( MODULES_BUILD_STATIC )
    message (" Internal modules will be built as shared components.")
endif ( MODULES_BUILD_STATIC )

## Make the user aware of what type they're building.
if (NOT CMAKE_BUILD_TYPE)
    message(" Build type: CMAKE_BUILD_TYPE is empty. Plain un-optimized build.")
else (NOT CMAKE_BUILD_TYPE)
    message(" Build type: " ${CMAKE_BUILD_TYPE} )          
endif (NOT CMAKE_BUILD_TYPE)
message("")

