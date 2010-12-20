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
## List of source files to compile
##
file (GLOB grepmidi_source_files
      grepmidi.cpp
      )

##
## Define target
##
add_executable ( grepmidi
      ${grepmidi_source_files}
      )

##
## Install location
##
install(TARGETS grepmidi 
      DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
      )
