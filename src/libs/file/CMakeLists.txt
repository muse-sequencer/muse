#=============================================================================
#  MusE
#  Linux Music Editor
#
#  file/CMakeLists.txt
#  (C) Copyright 2025 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
## Expand Qt macros in source files
##

QT5_WRAP_CPP (file_mocs
file.h
)

##
## List of source files to compile
##

file (GLOB file_source_files
      file.cpp
      )

##
## Define target
##

add_library ( file_module SHARED
      ${file_mocs}
      ${file_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${file_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##

set_target_properties( file_module
      PROPERTIES OUTPUT_NAME muse_file_module
      )

target_link_libraries(file_module
      ${QT_LIBRARIES}
      )

##
## Install location
##

install(TARGETS
        file_module
      DESTINATION ${MusE_MODULES_DIR}
      )
