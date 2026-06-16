# Install script for directory: /home/ad/doks/git/muse/src

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
   "/usr/local/share/doc/muse-4.2/AUTHORS;/usr/local/share/doc/muse-4.2/COPYING;/usr/local/share/doc/muse-4.2/ChangeLog;/usr/local/share/doc/muse-4.2/README;/usr/local/share/doc/muse-4.2/README.developer;/usr/local/share/doc/muse-4.2/README.developer.undo_system;/usr/local/share/doc/muse-4.2/README.ladspaguis;/usr/local/share/doc/muse-4.2/README.softsynth;/usr/local/share/doc/muse-4.2/README.vstsdk;/usr/local/share/doc/muse-4.2/README.win32;/usr/local/share/doc/muse-4.2/SECURITY;/usr/local/share/doc/muse-4.2/libdivide_LICENSE")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/AUTHORS"
    "/home/ad/doks/git/muse/src/COPYING"
    "/home/ad/doks/git/muse/src/ChangeLog"
    "/home/ad/doks/git/muse/src/README"
    "/home/ad/doks/git/muse/src/README.developer"
    "/home/ad/doks/git/muse/src/README.developer.undo_system"
    "/home/ad/doks/git/muse/src/README.ladspaguis"
    "/home/ad/doks/git/muse/src/README.softsynth"
    "/home/ad/doks/git/muse/src/README.vstsdk"
    "/home/ad/doks/git/muse/src/README.win32"
    "/home/ad/doks/git/muse/src/SECURITY"
    "/home/ad/doks/git/muse/src/libdivide_LICENSE"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ad/doks/git/muse/src/doc/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/libs/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/audio_convert/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/al/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/awl/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/grepmidi_qt/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/sandbox/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/man/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/plugins/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/muse/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/synti/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/packaging/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/utils/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/demos/cmake_install.cmake")
  include("/home/ad/doks/git/muse/src/share/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
