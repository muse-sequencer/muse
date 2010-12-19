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

file (GLOB SOURCE_FILES
      mess.cpp
      # mess2.cpp
      gui.cpp
      mono.cpp
      poly.cpp
      # midievent.cpp
      # Removed. Causing conflicts with /muse/mpevent
      ## mpevent.cpp
      )

add_library(synti SHARED
      ${PROJECT_BINARY_DIR}/all-pic.h.pch
      ${SOURCE_FILES}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

install(TARGETS synti DESTINATION ${MUSE_MODULES_DIR})

#
# -fPIC is necessary for 64 bit systems
#
set_target_properties( synti
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all-pic.h"
              OUTPUT_NAME muse_synti
   )

