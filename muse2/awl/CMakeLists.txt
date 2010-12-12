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

include(${PROJECT_SOURCE_DIR}/pch.txt)

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
      mslider midimslider midimeter combobox checkbox 
      posedit
      pitchedit 
      # poslabel 
      pitchlabel tempolabel 
      sigedit 
      # tcanvas
      tempoedit drawbar
      )

WRAP_H(mocH ${mocs})
WRAP_CPP(mocCPP ${mocs})

QT4_WRAP_CPP (moc_headers ${mocH})

set(awl_src
      awl.cpp utils.cpp  ${mocCPP} ${moc_headers}
      )

# TODO: Better filtering
file (GLOB SOURCE_FILES *.cpp *.h )
set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

add_library (
      awl
      STATIC
      ${awl_src}
      ${PROJECT_BINARY_DIR}/all.h.pch
      )

set_target_properties( awl
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
   )

# IF (APPLE)
#    message("Not yet able to build awlplugin under OS X.")
# ELSE (APPLE)
# 
#    #
#    #  build libawlplugin.so
#    #
#    QT4_WRAP_CPP (moc_awlplugin_headers
#       awlplugin.h
#       volknob.h knob.h aslider.h
#       slider.h volslider.h mslider.h
#       posedit.h poslabel.h
#       tempoedit.h tempolabel.h
#       combobox.h checkbox.h
#       panknob.h midipanknob.h
#       floatentry.h
#       drawbar.h
#       )
# 
#    add_library ( awlplugin SHARED
#        awlplugin.cpp
#        volknob.cpp knob.cpp aslider.cpp
#        slider.cpp volslider.cpp mslider.cpp
#        posedit.cpp poslabel.cpp
#        tempoedit.cpp tempolabel.cpp
#        combobox.cpp checkbox.cpp
#        panknob.cpp midipanknob.cpp
#        floatentry.cpp
#        drawbar.cpp
#        ${moc_awlplugin_headers}
#        ${PROJECT_BINARY_DIR}/all-pic.h.pch
#        )
# 
#    set_target_properties( awlplugin
#       PROPERTIES COMPILE_FLAGS
#          "-D_GNU_SOURCE -D_REENTRANT -DHAVE_CONFIG_H -DQT_PLUGIN -DQT_SHARED -DQT_NO_DEBUG -include ${PROJECT_BINARY_DIR}/all-pic.h"
#       )
#    target_link_libraries( awlplugin
#       al pthread asound
#       )
# 
#    install( TARGETS awlplugin DESTINATION ${QT_PLUGINS_DIR}/designer/ )
# 
#    add_executable( ltest ltest.cpp )
#    target_link_libraries ( ltest
#       awlplugin
#       ${QT_LIBRARIES}
#       )
# 
# ENDIF (APPLE)
