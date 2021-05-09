#!/bin/sh

PACKAGE_NAME=muse_master_amd64

echo building MusE package $PACKAGE_NAME
# Script to build a ubuntu .deb package of the current checkout.

# Step 1. build MusE
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=/opt/muse-sequencer.github.io ..
make

# Step 2. install a temporary installation (requires write access to opt) 
# TODO: install to a chroot environment
make install

# Step 3. Setup package environment
mkdir -p $PACKAGE_NAME/opt/muse-sequencer.github.io
mkdir -p $PACKAGE_NAME/usr/bin
mkdir -p $PACKAGE_NAME/usr/applications
mkdir -p $PACKAGE_NAME/usr/icons

ln -s /opt/muse-sequencer.github.io/bin/muse3 $PACKAGE_NAME/usr/bin/muse3

cp -r ../packaging/ubuntu/DEBIAN $PACKAGE_NAME
mkdir -p $PACKAGE_NAME/opt/muse-sequencer.github.io

cp -r /opt/muse-sequencer.github.io $PACKAGE_NAME/opt

cp packaging/io.github.muse_sequencer.Muse.desktop $PACKAGE_NAME/usr/applications/
cp ../packaging/io.github.muse_sequencer.Muse.png $PACKAGE_NAME/usr/icons/



# Step 4. create package
dpkg-deb --build $PACKAGE_NAME

