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

subdirs(
        libmidiplugin
        splitlayer
        filter 
	metronom
	transform
	trigg 
	dump
	drumglue
 )

set ( CMAKE_CXX_FLAGS 
      "${CMAKE_CXX_FLAGS} -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -fPIC"
      )

set ( CMAKE_LDD_FLAGS -module -avoid-version )
