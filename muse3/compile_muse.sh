#!/bin/bash
#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

if [ -d build ]; then
        echo "Build dir already exists"
else
	echo "Create build dir"
	mkdir build
fi
cd build

# to put the resulting binary in a specific location add -DCMAKE_INSTALL_PREFIX=<some location>
cmake -DCMAKE_BUILD_TYPE=release .. && make clean all && echo "Build was OK, now enter the 'build' dir and run 'make install' as root"

