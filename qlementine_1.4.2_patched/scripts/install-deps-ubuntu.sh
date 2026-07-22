#!/bin/bash

# General dependencies.
echo "Installing general dependencies..."
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config

# Qt dependencies.
echo "Installing Qt6 dependencies..."
sudo apt-get install -y \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-tools-dev \
  qt6-tools-dev-tools \
  qmake6 \
  libqt6svg6 \
  libqt6svg6-dev

echo "Done."
