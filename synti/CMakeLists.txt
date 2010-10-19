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

include_directories(
   .
   ${CMAKE_CURRENT_BINARY}
   ${PROJECT_SOURCE_DIR}
   ${PROJECT_SOURCE_DIR}/muse
   ${PROJECT_SOURCE_DIR}/muse/widgets
   ${PROJECT_SOURCE_DIR}/synti
   )

# set (SubDirs libsynti s1 organ deicsonze deicsonze2 simpledrums vam)
set (SubDirs libsynti s1 organ deicsonze simpledrums vam)

if (ENABLE_FLUID)
      set (SubDirs ${SubDirs} fluid fluidsynth )
endif (ENABLE_FLUID)

subdirs(${SubDirs})


