#=============================================================================
#  MusE
#  Linux Music Editor
#
#  plugin/CMakeLists.txt
#  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

##
## List of source files to compile
##

file (GLOB plugin_rdf_source_files
      plugin_rdf.cpp
      )
file (GLOB plugin_scan_source_files
      plugin_scan.cpp
      )
file (GLOB plugin_list_source_files
      plugin_list.cpp
      )
file (GLOB plugin_cache_reader_source_files
      plugin_cache_reader.cpp
      )
file (GLOB plugin_cache_writer_source_files
      plugin_cache_writer.cpp
      )

##
## Define target
##

add_library ( plugin_rdf_module SHARED
      ${plugin_rdf_source_files}
      )
add_library ( plugin_scan_module SHARED
      ${plugin_scan_source_files}
      )
add_library ( plugin_list_module SHARED
      ${plugin_list_source_files}
      )
add_library ( plugin_cache_reader_module SHARED
      ${plugin_cache_reader_source_files}
      )
add_library ( plugin_cache_writer_module SHARED
      ${plugin_cache_writer_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${plugin_scan_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##

set_target_properties( plugin_rdf_module
      PROPERTIES OUTPUT_NAME muse_plugin_rdf_module
      )
set_target_properties( plugin_scan_module
      PROPERTIES OUTPUT_NAME muse_plugin_scan_module
      )
set_target_properties( plugin_list_module
      PROPERTIES OUTPUT_NAME muse_plugin_list_module
      )
set_target_properties( plugin_cache_reader_module
      PROPERTIES OUTPUT_NAME muse_plugin_cache_reader_module
      )
set_target_properties( plugin_cache_writer_module
      PROPERTIES OUTPUT_NAME muse_plugin_cache_writer_module
      )

target_link_libraries(plugin_rdf_module
      ${QT_LIBRARIES}
      )
target_link_libraries(plugin_scan_module
      #xml_module
      ${QT_LIBRARIES}
      )
target_link_libraries(plugin_list_module
      #xml_module
      plugin_scan_module
      #${QT_LIBRARIES}
      )
target_link_libraries(plugin_cache_reader_module
      plugin_scan_module
      plugin_list_module
      xml_module
      #${LRDF_LIBRARIES}
      #${QT_LIBRARIES}
      )
target_link_libraries(plugin_cache_writer_module
      plugin_rdf_module
      plugin_scan_module
      plugin_list_module
      plugin_cache_reader_module
      xml_module
      ${LRDF_LIBRARIES}
      #${QT_LIBRARIES}
      )

if(LV2_SUPPORT)
      target_link_libraries(plugin_cache_writer_module ${LILV_LIBRARIES})
endif(LV2_SUPPORT)

##
## Install location
##

install(TARGETS
        plugin_rdf_module
        plugin_scan_module
        plugin_list_module
        plugin_cache_reader_module
        plugin_cache_writer_module
      DESTINATION ${MusE_MODULES_DIR}
      )
