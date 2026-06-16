# Install script for directory: /home/ad/doks/git/muse/src/libs

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ad/doks/git/muse/src/libs/evdata/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/memory/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/midi_controller/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/midnam/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/mpevent/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/string/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/sysex_helper/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/xml/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/time_stretch/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/wave/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/plugin/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/file/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/libs/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
