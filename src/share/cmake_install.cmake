# Install script for directory: /home/ad/doks/git/muse/src/share

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
   "/usr/local/share/muse-4.2/didyouknow.txt;/usr/local/share/muse-4.2/splash.jpg")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/share/didyouknow.txt"
    "/home/ad/doks/git/muse/src/share/splash.jpg"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ad/doks/git/muse/src/share/drummaps/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/instruments/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/plugins/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/pybridge/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/scripts/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/templates/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/wallpapers/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/scoreglyphs/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/note_names/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/locale/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/themes/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/metronome/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/rdf/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/share/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
