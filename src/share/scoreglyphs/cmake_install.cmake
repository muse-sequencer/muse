# Install script for directory: /home/ad/doks/git/muse/src/share/scoreglyphs

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
   "/usr/local/share/muse-4.2/scoreglyphs/0.png;/usr/local/share/muse-4.2/scoreglyphs/1.png;/usr/local/share/muse-4.2/scoreglyphs/2.png;/usr/local/share/muse-4.2/scoreglyphs/3.png;/usr/local/share/muse-4.2/scoreglyphs/4.png;/usr/local/share/muse-4.2/scoreglyphs/5.png;/usr/local/share/muse-4.2/scoreglyphs/6.png;/usr/local/share/muse-4.2/scoreglyphs/7.png;/usr/local/share/muse-4.2/scoreglyphs/8.png;/usr/local/share/muse-4.2/scoreglyphs/9.png;/usr/local/share/muse-4.2/scoreglyphs/acc_b.png;/usr/local/share/muse-4.2/scoreglyphs/acc_none.png;/usr/local/share/muse-4.2/scoreglyphs/acc_sharp.png;/usr/local/share/muse-4.2/scoreglyphs/clef_bass_big.png;/usr/local/share/muse-4.2/scoreglyphs/clef_violin_big.png;/usr/local/share/muse-4.2/scoreglyphs/dot.png;/usr/local/share/muse-4.2/scoreglyphs/flags16d.png;/usr/local/share/muse-4.2/scoreglyphs/flags16u.png;/usr/local/share/muse-4.2/scoreglyphs/flags32d.png;/usr/local/share/muse-4.2/scoreglyphs/flags32u.png;/usr/local/share/muse-4.2/scoreglyphs/flags64d.png;/usr/local/share/muse-4.2/scoreglyphs/flags64u.png;/usr/local/share/muse-4.2/scoreglyphs/flags8d.png;/usr/local/share/muse-4.2/scoreglyphs/flags8u.png;/usr/local/share/muse-4.2/scoreglyphs/half.png;/usr/local/share/muse-4.2/scoreglyphs/quarter.png;/usr/local/share/muse-4.2/scoreglyphs/rest1.png;/usr/local/share/muse-4.2/scoreglyphs/rest16.png;/usr/local/share/muse-4.2/scoreglyphs/rest2.png;/usr/local/share/muse-4.2/scoreglyphs/rest32.png;/usr/local/share/muse-4.2/scoreglyphs/rest4.png;/usr/local/share/muse-4.2/scoreglyphs/rest8.png;/usr/local/share/muse-4.2/scoreglyphs/whole.png")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2/scoreglyphs" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/share/scoreglyphs/0.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/1.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/2.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/3.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/4.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/5.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/6.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/7.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/8.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/9.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/acc_b.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/acc_none.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/acc_sharp.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/clef_bass_big.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/clef_violin_big.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/dot.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags16d.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags16u.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags32d.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags32u.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags64d.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags64u.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags8d.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/flags8u.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/half.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/quarter.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest1.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest16.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest2.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest32.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest4.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/rest8.png"
    "/home/ad/doks/git/muse/src/share/scoreglyphs/whole.png"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/share/scoreglyphs/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
