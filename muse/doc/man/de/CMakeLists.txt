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

set (SRC man-de.tex)

add_custom_command ( 
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/man-de.pdf
      DEPENDS 
            ${CMAKE_CURRENT_SOURCE_DIR}/man-de.tex
            ${CMAKE_CURRENT_SOURCE_DIR}/projekte.tex
      COMMAND ${TEXEXEC_PATH}/texexec 
      ARGS  --language=en --verbose --batch --pdf
            --path=${CMAKE_CURRENT_SOURCE_DIR} 
            --result=${CMAKE_CURRENT_BINARY_DIR}/man-de.pdf 
            man-de.tex
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )

add_custom_target ( man-de ALL 
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/man-de.pdf 
      )

install_files ( /share/${MusE_INSTALL_NAME}/doc/ .pdf 
   ${CMAKE_CURRENT_BINARY_DIR}/man-de.pdf )

set (extraClean man-de.log man-de.tmp man-de.tuo man-de.tui
   man-de-mpgraph.mp mpgraph.mp )

set_directory_properties( PROPERTIES
   ADDITIONAL_MAKE_CLEAN_FILES "${extraClean}"
   )
