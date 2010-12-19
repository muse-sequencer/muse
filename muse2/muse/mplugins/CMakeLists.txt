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

# subdirs(
#         libmidiplugin
#         splitlayer
#         filter 
#       metronom
#       transform
#       trigg 
#       dump
#       drumglue
#  )
# 
# set ( CMAKE_CXX_FLAGS 
#       "${CMAKE_CXX_FLAGS} -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -fPIC"
#       )
# 
# set ( CMAKE_LDD_FLAGS -module -avoid-version )


file (GLOB UI_FILES midifilter.ui mrconfigbase.ui )
QT4_WRAP_UI ( mplugins_uis ${UI_FILES} )

if ( ENABLE_EXPERIMENTAL )
      QT4_WRAP_UI ( mplugins_experimental_uis rhythmbase.ui)
      set ( experimental_hdrs rhythm.h )
      set ( experimental_srcs
            rhythm.cpp
            ${mplugins_experimental_uis}
            )
endif ( ENABLE_EXPERIMENTAL )

QT4_WRAP_CPP ( mplugins_mocs  mittranspose.h midiitransform.h midifilterimpl.h mrconfig.h ${experimental_hdrs} )

file (GLOB SOURCE_FILES
      mitplugin.cpp mitplugin.h 
      mittranspose.cpp 
      midiitransform.cpp 
      midifilterimpl.cpp 
      mrconfig.cpp 
      random.cpp random.h 
      )

add_library ( mplugins SHARED
      ${SOURCE_FILES}
      ${mplugins_mocs}
      ${mplugins_uis}
      ${experimental_srcs}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

install(TARGETS mplugins DESTINATION ${MUSE_MODULES_DIR})

add_dependencies(mplugins widgets)

target_link_libraries( mplugins
   # midiplugin
   ${QT_LIBRARIES}
   widgets
   )

# - tell cmake to name target transform.so instead of 
#   libtransform.so
# - use precompiled header files
#
set_target_properties ( mplugins
   # PROPERTIES PREFIX ""
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
              OUTPUT_NAME muse_mplugins
   )

# install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/midiplugins/ transform )

