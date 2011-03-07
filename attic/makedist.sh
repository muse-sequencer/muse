#!/bin/sh

echo ""
echo "This script creates a distribution from a subversion checkout of MusE source tree."
echo ""
echo "The script simply tars together the entire source dir after removing some redundant"
echo "files, like .svn. This means that this script should ONLY be executed on a clean"
echo "checkout which does not contain any build files or other intermediate junk."
echo ""
echo "Press ENTER to continue, otherwise Ctrl-C."
echo ""


TARGET=$(grep "SET(MusE_INSTALL_NAME" muse/CMakeLists.txt | cut -d\" -f 2)
echo "Distribution name: $TARGET"
read
rm -rf $TARGET
cp -r muse $TARGET

find $TARGET -name .svn -exec rm -rf {} \;
tar cvfz $TARGET.tar.gz $TARGET
