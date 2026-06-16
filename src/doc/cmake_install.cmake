# Install script for directory: /home/ad/doks/git/muse/src/doc

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
   "/usr/local/share/doc/muse-4.2/muse_pdf/documentation.pdf;/usr/local/share/doc/muse-4.2/muse_pdf/developer_docs.pdf")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2/muse_pdf" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/doc/pdf/documentation.pdf"
    "/home/ad/doks/git/muse/src/doc/pdf/developer_docs.pdf"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/doc/muse-4.2/muse_html/single/documentation/arrow_tool.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/documentation.css;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/documentation.html;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/img1.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/index.html;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_add_track.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_annotated.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_with_arrangement.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_with_automation.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_with_midi_automation.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/main_window_with_midi_editor_vam.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/midi_config_window.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/midi_routing_matrix.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/mixer.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/mixer_with_one_input.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/mixer_with_one_input_buttons.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/muse2.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/no_audio.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/output_routing.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/project_my_first_song.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/soloing_window.png;/usr/local/share/doc/muse-4.2/muse_html/single/documentation/vam_synth.png")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2/muse_html/single/documentation" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/arrow_tool.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/documentation.css"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/documentation.html"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/img1.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/index.html"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_add_track.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_annotated.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_with_arrangement.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_with_automation.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_with_midi_automation.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/main_window_with_midi_editor_vam.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/midi_config_window.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/midi_routing_matrix.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/mixer.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/mixer_with_one_input.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/mixer_with_one_input_buttons.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/muse2.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/no_audio.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/output_routing.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/project_my_first_song.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/soloing_window.png"
    "/home/ad/doks/git/muse/src/doc/html/single/documentation/vam_synth.png"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/developer_docs.css;/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/developer_docs.html;/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/img1.png;/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/img2.png;/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/img3.png;/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs/index.html")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2/muse_html/single/developer_docs" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/developer_docs.css"
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/developer_docs.html"
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/img1.png"
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/img2.png"
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/img3.png"
    "/home/ad/doks/git/muse/src/doc/html/single/developer_docs/index.html"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/doc/muse-4.2/muse_html/split/documentation/arrow_tool.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/documentation.css;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/documentation.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/img1.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/index.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_add_track.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_annotated.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_with_arrangement.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_with_automation.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_with_midi_automation.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/main_window_with_midi_editor_vam.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/midi_config_window.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/midi_routing_matrix.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/mixer.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/mixer_with_one_input.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/mixer_with_one_input_buttons.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/muse2.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/no_audio.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node1.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node10.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node11.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node12.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node13.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node2.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node3.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node4.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node5.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node6.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node7.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node8.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/node9.html;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/output_routing.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/project_my_first_song.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/soloing_window.png;/usr/local/share/doc/muse-4.2/muse_html/split/documentation/vam_synth.png")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2/muse_html/split/documentation" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/arrow_tool.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/documentation.css"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/documentation.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/img1.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/index.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_add_track.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_annotated.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_with_arrangement.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_with_automation.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_with_midi_automation.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/main_window_with_midi_editor_vam.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/midi_config_window.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/midi_routing_matrix.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/mixer.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/mixer_with_one_input.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/mixer_with_one_input_buttons.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/muse2.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/no_audio.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node1.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node10.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node11.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node12.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node13.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node2.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node3.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node4.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node5.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node6.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node7.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node8.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/node9.html"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/output_routing.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/project_my_first_song.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/soloing_window.png"
    "/home/ad/doks/git/muse/src/doc/html/split/documentation/vam_synth.png"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/developer_docs.css;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/developer_docs.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/img1.png;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/img2.png;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/img3.png;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/index.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node1.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node10.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node11.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node12.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node13.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node14.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node15.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node2.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node3.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node4.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node5.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node6.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node7.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node8.html;/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs/node9.html")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/share/doc/muse-4.2/muse_html/split/developer_docs" TYPE FILE FILES
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/developer_docs.css"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/developer_docs.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/img1.png"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/img2.png"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/img3.png"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/index.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node1.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node10.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node11.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node12.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node13.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node14.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node15.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node2.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node3.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node4.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node5.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node6.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node7.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node8.html"
    "/home/ad/doks/git/muse/src/doc/html/split/developer_docs/node9.html"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/ad/doks/git/muse/src/doc/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
