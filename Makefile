#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2008 by Werner Schweer and others
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

CPUS     = `grep -c processor /proc/cpuinfo`
PREFIX   = "/usr/local"
VERSION  = "muse-2.0.0"

release:
	if test ! -d build;                         \
         then                                       \
            mkdir build;                            \
            cd build;                               \
            cmake -DCMAKE_BUILD_TYPE=RELEASE	    \
            	  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
            	   ../muse; 			    \
            make -j ${CPUS};                        \
         else                                       \
            echo "build directory does already exist, please remove first with 'make clean'";       \
         fi;

debug:
	if test ! -d build;                           \
         then                                       \
            mkdir build;                            \
            cd build;                               \
            cmake -DCMAKE_BUILD_TYPE=DEBUG	    \
            	  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
            	   ../muse; 			    \
            make -j ${CPUS};                        \
         else                                       \
            echo "build directory does already exist, please remove first with 'make clean'";       \
         fi

#
# clean out of source build
#

clean:
	-rm -rf build

#
# dist
#     create source distribution
#     - get current version from sourceforge
#     - remove .svn directories
#     - tar
#

dist:
	-rm -rf muse.dist
	mkdir muse.dist
	cd muse.dist; svn co https://lmuse.svn.sourceforge.net/svnroot/muse/trunk ${VERSION}
	cd muse.dist; find . -name .svn -print0 | xargs -0 /bin/rm -rf
	cd muse.dist; tar cvfj ${VERSION}.tar.bz2 ${VERSION}
	mv muse.dist/${VERSION}.tar.bz2 .

install:
	cd build; make install

#
# this creates a shell archive / installer for
#     Mscore binary
#

package:
	cd build; make package

man:
	cd build; make man

