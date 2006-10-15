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

add_library ( doublechorus SHARED
      doublechorus.cpp doublechorusmodel.cpp simplechorusmodel.cpp
      )

# tell cmake to name the target doublechorus.so instead of
# libdoublechorus.so
#
set_target_properties (doublechorus
      PROPERTIES PREFIX ""
      #COMPILE_FLAGS "-O3"
      )

install_targets ( /lib/${MusE_INSTALL_NAME}/plugins doublechorus)
