#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
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
QT4_WRAP_CPP (awl_mocs
      aslider.h
      # awlplugin.h
      checkbox.h
      combobox.h
      drawbar.h
      floatentry.h
      knob.h
      midimeter.h
      midimslider.h
      midipanentry.h
      midipanknob.h
      midivolentry.h
      mslider.h
      panentry.h
      panknob.h
      pitchedit.h
      pitchlabel.h
      posedit.h
      # poslabel.h
      sigedit.h
      sigspinbox.h
      slider.h
      # tcanvas.h
      tempoedit.h
      tempolabel.h
      volentry.h
      volknob.h
      volslider.h
      )

##
## List of source files to compile
##
file (GLOB awl_source_files
      aslider.cpp
      awl.cpp
      # awlplugin.cpp
      checkbox.cpp
      combobox.cpp
      drawbar.cpp
      floatentry.cpp
      knob.cpp
      # ltest.cpp
      midimeter.cpp
      midimslider.cpp
      midipanentry.cpp
      midipanknob.cpp
      midivolentry.cpp
      mslider.cpp
      panentry.cpp
      panknob.cpp
      pitchedit.cpp
      pitchlabel.cpp
      posedit.cpp
      # poslabel.cpp
      sigedit.cpp
      sigspinbox.cpp
      slider.cpp
      # tcanvas.cpp
      tempoedit.cpp
      tempolabel.cpp
      utils.cpp
      volentry.cpp
      volknob.cpp
      volslider.cpp
      )

##
## Define target
##
add_library ( awl ${MODULES_BUILD}
      ${awl_source_files}
      ${awl_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${awl_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( awl
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_awl
      )

##
## Linkage
##
target_link_libraries ( awl
      ${QT_LIBRARIES}
      al
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS awl
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
