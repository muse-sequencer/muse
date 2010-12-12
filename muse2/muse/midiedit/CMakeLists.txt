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

QT4_WRAP_CPP ( midiedit_mocs
#       midicmd.h
#       gatetime.h
#       velocity.h
#       quantconfig.h
#       dcanvas.h
#       drumedit.h
#       ecanvas.h
#       midieditor.h
#       pianoroll.h
#       prcanvas.h
#       miditracker.h
#       trackpattern.h
      
      pianoroll.h 
      piano.h 
      prcanvas.h 
      quantconfig.h 
      drumedit.h 
      dlist.h 
      dcanvas.h 
      ecanvas.h
      )

file (GLOB SOURCE_FILES
#      midicmd.h
#      gatetime.h
#       midicmd.cpp
#       gatetime.cpp
#       velocity.cpp
#       quantconfig.cpp
#       citem.h
#       cmd.h
#       drummap.h
#       midicmd.h
#       citem.cpp
#       dcanvas.cpp
#       drumedit.cpp
#       drummap.cpp
#       ecanvas.cpp
#       midieditor.cpp
#       pianoroll.cpp
#       prcanvas.cpp
#       miditracker.cpp
#       trackpattern.cpp
      
      drummap.cpp drummap.h 
      cmd.h 
      pianoroll.cpp # pianoroll.h 
      piano.cpp # piano.h 
      prcanvas.cpp # prcanvas.h 
      quantconfig.cpp # quantconfig.h 
      drumedit.cpp # drumedit.h 
      dlist.cpp # dlist.h 
      dcanvas.cpp # dcanvas.h 
      ecanvas.cpp # ecanvas.h
      )

add_library ( midiedit STATIC
      ${SOURCE_FILES}
      # ${midiedit_ui_headers}
      ${midiedit_mocs}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

add_dependencies(midiedit widgets)

set_target_properties( midiedit
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      )

