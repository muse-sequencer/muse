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

default:
	if test ! -d build;                           \
         then                                       \
            echo "+creating build directory";       \
            mkdir build;                            \
            echo "+entering build directory";       \
            cd build;                               \
            echo "+calling cmake" ;                 \
            cmake ../muse ;                         \
         else                                       \
            echo "+entering build directory";       \
            cd build;                               \
         fi;                                        \
      echo "+start top level make...";              \
      make -f Makefile


#
# clean out of source build
#

clean:
	-rm -rf build

#
# create source distribution
#

dist:
	cd build; make package_source
	mv build/muse-*.tar.gz .

install:
	cd build; make install

#
# this creates a shell archive / installer for
#     MusE binary
#

package:
	cd build; make package
	mv build/muse-*.sh .


