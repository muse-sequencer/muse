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

MACRO(WRAP_CPP outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.cpp)
      ENDFOREACH(it)
   ENDMACRO(WRAP_CPP)

MACRO(WRAP_H outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.h)
      ENDFOREACH(it)
   ENDMACRO(WRAP_H)

set(mocs
      aslider knob panknob slider volknob volslider floatentry
      volentry panentry midivolentry midipanentry midipanknob
      mslider midimslider midimeter combobox checkbox posedit
      pitchedit poslabel pitchlabel tempolabel sigedit tcanvas
      tempoedit
      )

WRAP_H(mocH ${mocs})
WRAP_CPP(mocCPP ${mocs})

QT4_WRAP_CPP (moc_headers ${mocH})

set ( awl_src awl.cpp utils.cpp  ${mocCPP} ${moc_headers} )

add_library ( awl STATIC ${awl_src} )
set_target_properties( awl
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
   )

add_library ( awlplugin SHARED  ${awl_src} awlplugin.cpp )
set_target_properties( awlplugin
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all-pic.h"
   )

