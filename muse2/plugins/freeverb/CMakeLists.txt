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
file (GLOB freeverb_source_files
      freeverb.cpp
      revmodel.cpp
      )

##
## Define target
##
add_library ( freeverb SHARED
      ${freeverb_source_files}
      )

##
## Compilation flags and target name
##
# tell cmake to name the target freeverb.so instead of
# libfreeverb.so
#
set_target_properties (freeverb
      PROPERTIES PREFIX ""
      COMPILE_FLAGS "-O2"
      )

##
## Install location
##
install( TARGETS freeverb
      DESTINATION ${MusE_PLUGINS_DIR}
      )
install( FILES readme.txt
      DESTINATION ${MusE_DOC_DIR}/freeverb
      )

