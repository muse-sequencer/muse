# Install script for directory: /home/ad/doks/git/muse/src/share/instruments

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
   "/usr/local/share/muse-4.2/instruments/Access_Virus.idf;/usr/local/share/muse-4.2/instruments/Akai-SG01v.idf;/usr/local/share/muse-4.2/instruments/Alesis-QS-78R.idf;/usr/local/share/muse-4.2/instruments/AlesisQS6.idf;/usr/local/share/muse-4.2/instruments/Casio_CDP-S360.idf;/usr/local/share/muse-4.2/instruments/Classic_cantabile_sp-250.idf;/usr/local/share/muse-4.2/instruments/Drumgizmo - CrocellKit.idf;/usr/local/share/muse-4.2/instruments/Edirol-SD90.idf;/usr/local/share/muse-4.2/instruments/Emu-4mbgsgmmt-sf.idf;/usr/local/share/muse-4.2/instruments/GeneralUser_GS.idf;/usr/local/share/muse-4.2/instruments/Hammond_XB-1.idf;/usr/local/share/muse-4.2/instruments/KORG microSTATION combinations.idf;/usr/local/share/muse-4.2/instruments/KORG microSTATION.idf;/usr/local/share/muse-4.2/instruments/Korg Krome (Bank Map GM2).idf;/usr/local/share/muse-4.2/instruments/Korg Krome (Bank Map KORG).idf;/usr/local/share/muse-4.2/instruments/Korg Wavestation EX.idf;/usr/local/share/muse-4.2/instruments/Korg-MS2000R.idf;/usr/local/share/muse-4.2/instruments/Korg-X50.idf;/usr/local/share/muse-4.2/instruments/Korg-X5DR-PresetA.idf;/usr/local/share/muse-4.2/instruments/Korg-X5DR-PresetB.idf;/usr/local/share/muse-4.2/instruments/Kurzweil-SP2X.idf;/usr/local/share/muse-4.2/instruments/Lexicon-MX200.idf;/usr/local/share/muse-4.2/instruments/MC303.idf;/usr/local/share/muse-4.2/instruments/MC505.idf;/usr/local/share/muse-4.2/instruments/Roland INTEGRA-7.idf;/usr/local/share/muse-4.2/instruments/Roland SD-50.idf;/usr/local/share/muse-4.2/instruments/Roland-E28.idf;/usr/local/share/muse-4.2/instruments/Roland-JV90.idf;/usr/local/share/muse-4.2/instruments/Roland-MT32.idf;/usr/local/share/muse-4.2/instruments/Roland-SC55mkII.idf;/usr/local/share/muse-4.2/instruments/Roland-SC88.idf;/usr/local/share/muse-4.2/instruments/Roland-SCD70.idf;/usr/local/share/muse-4.2/instruments/Roland-XP30.idf;/usr/local/share/muse-4.2/instruments/Roland_FantomXR.idf;/usr/local/share/muse-4.2/instruments/Roland_SC-88Pro.idf;/usr/local/share/muse-4.2/instruments/Roland_SRX-02.idf;/usr/local/share/muse-4.2/instruments/Roland_SRX-09.idf;/usr/local/share/muse-4.2/instruments/Virtuosity-Drums.idf;/usr/local/share/muse-4.2/instruments/Waldorf-Q.idf;/usr/local/share/muse-4.2/instruments/Waldorf-microQ-Factory2000.idf;/usr/local/share/muse-4.2/instruments/Waldorf-microQ-Factory2001.idf;/usr/local/share/muse-4.2/instruments/Waldorf-microQ-Phoenix.idf;/usr/local/share/muse-4.2/instruments/Waldorf-microQ.idf;/usr/local/share/muse-4.2/instruments/Waldorf_Microwave-I.idf;/usr/local/share/muse-4.2/instruments/Yamaha-01v.idf;/usr/local/share/muse-4.2/instruments/Yamaha-9000pro.idf;/usr/local/share/muse-4.2/instruments/Yamaha-CS1x.idf;/usr/local/share/muse-4.2/instruments/Yamaha-MX49_MX61.idf;/usr/local/share/muse-4.2/instruments/Yamaha-Motif XS.idf;/usr/local/share/muse-4.2/instruments/Yamaha-Motif-Rack.idf;/usr/local/share/muse-4.2/instruments/Yamaha-Motif.idf;/usr/local/share/muse-4.2/instruments/Yamaha-P100.idf;/usr/local/share/muse-4.2/instruments/Yamaha-P50m.idf;/usr/local/share/muse-4.2/instruments/Yamaha-PSR275.idf;/usr/local/share/muse-4.2/instruments/Yamaha-PSR530.idf;/usr/local/share/muse-4.2/instruments/Yamaha-Rex50.idf;/usr/local/share/muse-4.2/instruments/Yamaha-S30_S80.idf;/usr/local/share/muse-4.2/instruments/Yamaha-S90.idf;/usr/local/share/muse-4.2/instruments/ZynAdd-1_4.idf;/usr/local/share/muse-4.2/instruments/emuproteus2000.idf;/usr/local/share/muse-4.2/instruments/gm.idf;/usr/local/share/muse-4.2/instruments/gm2.idf;/usr/local/share/muse-4.2/instruments/gs.idf;/usr/local/share/muse-4.2/instruments/ns5r.idf;/usr/local/share/muse-4.2/instruments/xg.idf;/usr/local/share/muse-4.2/instruments/yam_mo6_v4.idf")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/muse-4.2/instruments" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/share/instruments/Access_Virus.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Akai-SG01v.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Alesis-QS-78R.idf"
    "/home/ad/doks/git/muse/src/share/instruments/AlesisQS6.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Casio_CDP-S360.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Classic_cantabile_sp-250.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Drumgizmo - CrocellKit.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Edirol-SD90.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Emu-4mbgsgmmt-sf.idf"
    "/home/ad/doks/git/muse/src/share/instruments/GeneralUser_GS.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Hammond_XB-1.idf"
    "/home/ad/doks/git/muse/src/share/instruments/KORG microSTATION combinations.idf"
    "/home/ad/doks/git/muse/src/share/instruments/KORG microSTATION.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg Krome (Bank Map GM2).idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg Krome (Bank Map KORG).idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg Wavestation EX.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg-MS2000R.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg-X50.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg-X5DR-PresetA.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Korg-X5DR-PresetB.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Kurzweil-SP2X.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Lexicon-MX200.idf"
    "/home/ad/doks/git/muse/src/share/instruments/MC303.idf"
    "/home/ad/doks/git/muse/src/share/instruments/MC505.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland INTEGRA-7.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland SD-50.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-E28.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-JV90.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-MT32.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-SC55mkII.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-SC88.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-SCD70.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland-XP30.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland_FantomXR.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland_SC-88Pro.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland_SRX-02.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Roland_SRX-09.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Virtuosity-Drums.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf-Q.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf-microQ-Factory2000.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf-microQ-Factory2001.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf-microQ-Phoenix.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf-microQ.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Waldorf_Microwave-I.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-01v.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-9000pro.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-CS1x.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-MX49_MX61.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-Motif XS.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-Motif-Rack.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-Motif.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-P100.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-P50m.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-PSR275.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-PSR530.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-Rex50.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-S30_S80.idf"
    "/home/ad/doks/git/muse/src/share/instruments/Yamaha-S90.idf"
    "/home/ad/doks/git/muse/src/share/instruments/ZynAdd-1_4.idf"
    "/home/ad/doks/git/muse/src/share/instruments/emuproteus2000.idf"
    "/home/ad/doks/git/muse/src/share/instruments/gm.idf"
    "/home/ad/doks/git/muse/src/share/instruments/gm2.idf"
    "/home/ad/doks/git/muse/src/share/instruments/gs.idf"
    "/home/ad/doks/git/muse/src/share/instruments/ns5r.idf"
    "/home/ad/doks/git/muse/src/share/instruments/xg.idf"
    "/home/ad/doks/git/muse/src/share/instruments/yam_mo6_v4.idf"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/share/instruments/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
