# Install script for directory: /home/ad/doks/git/muse/src/share/scripts

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/muse-4.2/scripts/ConstantLength;/usr/local/share/muse-4.2/scripts/ConstantVelocityForNote;/usr/local/share/muse-4.2/scripts/CreateBassline;/usr/local/share/muse-4.2/scripts/DoNothing;/usr/local/share/muse-4.2/scripts/Humanize;/usr/local/share/muse-4.2/scripts/RandomPosition1;/usr/local/share/muse-4.2/scripts/RandomizeVelocityRelative;/usr/local/share/muse-4.2/scripts/RemoveAftertouch;/usr/local/share/muse-4.2/scripts/RemoveShortEvents;/usr/local/share/muse-4.2/scripts/Reverse;/usr/local/share/muse-4.2/scripts/Rhythm1;/usr/local/share/muse-4.2/scripts/SpeedChange;/usr/local/share/muse-4.2/scripts/SpeedDouble;/usr/local/share/muse-4.2/scripts/SpeedHalf;/usr/local/share/muse-4.2/scripts/SwingQuantize1;/usr/local/share/muse-4.2/scripts/TempoDelay")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2/scripts" TYPE PROGRAM FILES
    "/home/ad/doks/git/muse/src/share/scripts/ConstantLength"
    "/home/ad/doks/git/muse/src/share/scripts/ConstantVelocityForNote"
    "/home/ad/doks/git/muse/src/share/scripts/CreateBassline"
    "/home/ad/doks/git/muse/src/share/scripts/DoNothing"
    "/home/ad/doks/git/muse/src/share/scripts/Humanize"
    "/home/ad/doks/git/muse/src/share/scripts/RandomPosition1"
    "/home/ad/doks/git/muse/src/share/scripts/RandomizeVelocityRelative"
    "/home/ad/doks/git/muse/src/share/scripts/RemoveAftertouch"
    "/home/ad/doks/git/muse/src/share/scripts/RemoveShortEvents"
    "/home/ad/doks/git/muse/src/share/scripts/Reverse"
    "/home/ad/doks/git/muse/src/share/scripts/Rhythm1"
    "/home/ad/doks/git/muse/src/share/scripts/SpeedChange"
    "/home/ad/doks/git/muse/src/share/scripts/SpeedDouble"
    "/home/ad/doks/git/muse/src/share/scripts/SpeedHalf"
    "/home/ad/doks/git/muse/src/share/scripts/SwingQuantize1"
    "/home/ad/doks/git/muse/src/share/scripts/TempoDelay"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/share/scripts/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
