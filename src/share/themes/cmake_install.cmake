# Install script for directory: /home/ad/doks/git/muse/src/share/themes

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
   "/usr/local/share/muse-4.2/themes/Ardour.cfc;/usr/local/share/muse-4.2/themes/Ardour.qss;/usr/local/share/muse-4.2/themes/Dark Flat.cfc;/usr/local/share/muse-4.2/themes/Dark Flat.qss;/usr/local/share/muse-4.2/themes/Dark Theme.cfc;/usr/local/share/muse-4.2/themes/Dark Theme.qss;/usr/local/share/muse-4.2/themes/Deep Ocean.cfc;/usr/local/share/muse-4.2/themes/Deep Ocean.qss;/usr/local/share/muse-4.2/themes/Light Theme.cfc;/usr/local/share/muse-4.2/themes/Light Theme.qss")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2/themes" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/share/themes/Ardour.cfc"
    "/home/ad/doks/git/muse/src/share/themes/Ardour.qss"
    "/home/ad/doks/git/muse/src/share/themes/Dark Flat.cfc"
    "/home/ad/doks/git/muse/src/share/themes/Dark Flat.qss"
    "/home/ad/doks/git/muse/src/share/themes/Dark Theme.cfc"
    "/home/ad/doks/git/muse/src/share/themes/Dark Theme.qss"
    "/home/ad/doks/git/muse/src/share/themes/Deep Ocean.cfc"
    "/home/ad/doks/git/muse/src/share/themes/Deep Ocean.qss"
    "/home/ad/doks/git/muse/src/share/themes/Light Theme.cfc"
    "/home/ad/doks/git/muse/src/share/themes/Light Theme.qss"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/muse-4.2/themes//Deep Ocean")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2/themes/" TYPE DIRECTORY FILES "/home/ad/doks/git/muse/src/share/themes/Deep Ocean")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/share/themes/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
