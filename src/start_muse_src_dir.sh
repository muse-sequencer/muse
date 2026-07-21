#!/bin/bash

# NOTE: fix python path below, path that has libpython3.XX.so (version that is linked by MusE)
 
cd ../build_release/

QT_QPA_PLATFORM=xcb \
 \
LD_LIBRARY_PATH=\
/home/.pyenv/versions/3.12.8/lib/:\
muse/:\
libs/file:\
libs/plugin/:\
libs/midnam/:\
libs/midi_controller/:\
libs/mpevent/:\
libs/evdata/:\
libs/memory/:\
libs/sysex_helper/:\
libs/xml/  \
 \
./muse/muse4  "$@"
 
