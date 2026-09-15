#!/bin/bash

echo ""
echo " ==================================== "
echo "   Running cmake (configuration)"
echo " ==================================== "
cmake -S . -B ../build_release  -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DMODULES_BUILD_STATIC=ON  -DMUSE_ENABLE_ASAN=OFF
# to put the resulting binary in a specific location add -DCMAKE_INSTALL_PREFIX=<some location>


echo ""
echo " ==================================== "
echo "     Starting the build process"
echo " ==================================== "

cmake --build ../build_release -j7


echo ""
echo " ==================================== "
# echo "   # after successful compilation, install with: "
# echo "   sudo  make -C../build  install  "
# echo " ==================================== "



echo -n "Do you want to run 'make install' command ? (to /usr/local/ by default) Y/N: "
read -r answer

case "$answer" in
    [Yy]) echo "Starting installation. " && sudo  cmake --install ../build_release  ;;  # optional: --prefix /opt/muse
    [Nn]) echo "==> NO installation. Ending. " ;;
    *)    echo "==> Invalid Input. NOT installed. " ;;
esac


echo ""
echo " Finished."
echo " ==================================== "
