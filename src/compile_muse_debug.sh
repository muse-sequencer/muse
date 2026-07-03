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


echo ""
echo " ==================================== "
echo "   Running cmake (configuration)"
echo " ==================================== "
cmake -B ../build  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMODULES_BUILD_STATIC=ON 
# to put the resulting binary in a specific location add -DCMAKE_INSTALL_PREFIX=<some location>


echo ""
echo " ==================================== "
echo "     Starting the build process"
echo " ==================================== "

cmake --build ../build -j4


echo ""
echo " ==================================== "
# echo "   # after successful compilation, install with: "
# echo "   sudo  make -C../build  install  "
# echo " ==================================== "



echo -n "Do you want to run 'make install' command ? (/usr/local/ by default) Y/N: "
read -r answer

case "$answer" in
    [Yy]) echo "Starting installation. " && sudo make -C../build install  ;;
    [Nn]) echo "==> NO installation. Ending. " ;;
    *)    echo "==> Invalid Input. NOT installed. " ;;
esac


echo ""
echo " Finished."
echo " ==================================== "


# REMOVE:
# && make clean all && echo "Build was OK, now enter the 'build' dir and run 'make install' as root"
