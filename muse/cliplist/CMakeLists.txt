#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP (cliplist_mocs
      cliplist.h
      )

##
## List of source files to compile
##
file (GLOB cliplist_source_files
      cliplist.cpp 
      )

##
## Define target
##
add_library ( cliplist SHARED
      ${cliplist_source_files}
      ${cliplist_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${cliplist_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( cliplist
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
      OUTPUT_NAME muse_cliplist
      )

##
## Linkage
##
target_link_libraries ( cliplist
      ${QT_LIBRARIES}
      awl
      )

##
## Install location
##
install(TARGETS cliplist
      DESTINATION ${MusE_MODULES_DIR}
      )


