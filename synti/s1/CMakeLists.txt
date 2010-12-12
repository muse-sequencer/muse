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

file (GLOB SOURCE_FILES s1.cpp )
add_library ( s1 SHARED ${SOURCE_FILES} )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

# target_link_libraries( s1 synti )

# tell cmake to name target s1.so instead of 
# libs1.so
#
set_target_properties ( s1 
  PROPERTIES PREFIX "" 
  COMPILE_FLAGS "-fvisibility=hidden"
  )

target_link_libraries(s1
      synti
      )
install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ s1 )

