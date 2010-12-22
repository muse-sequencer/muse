#!/bin/bash

if [ -d build ]; then
        echo "Build dir already exists"
else
	echo "Create build dir"
	mkdir build
fi
cd build
cmake -DCMAKE_BUILD_TYPE=release .. && make && echo "Build was OK, now enter the 'build' dir and run 'make install' as root"

