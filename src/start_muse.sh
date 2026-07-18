#!/bin/bash

### This script starts muse in Xwayland/XCB mode, by default from /usr/local/bin/muse4 , with jack-audio driver
### (needs compilation first.)

### get script directory
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# echo "This bash script is located in: $SCRIPT_DIR"


### test for virtual python environment , set QT paths
### WARNING: venv could mix qt5 (linked in muse) with qt6 (from env) in same namespace, creating errors
# if [ -n "$VIRTUAL_ENV" ]; then
#     PYVER=$(python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
#     echo "INFO: using py venv: ${PYVER} "
# 
#     export QT_PLUGIN_PATH="$VIRTUAL_ENV/lib/python$PYVER/site-packages/PyQt6/Qt6/plugins:/usr/lib/qt6/plugins"
#     export LD_LIBRARY_PATH="$VIRTUAL_ENV/lib/python$PYVER/site-packages/PyQt6/Qt6/lib"
# fi


### on wayland, use XCB/Xwayland (for better plugin-GUI support)
export QT_QPA_PLATFORM=xcb
export GDK_BACKEND=x11
export SDL_VIDEODRIVER=x11
export CLUTTER_BACKEND=x11


### optional preload
#export LD_PRELOAD=/usr/lib/libasan.so:/usr/local/lib/libinstpatch-1.0.so 


### optional, use lsan suppression file 
SUPP_LSAN="${SCRIPT_DIR}/misc/lsan.supp"
# HINT: For debugging, set LSAN_OPTIONS (leak sanitizer) environment variable.
#       export LSAN_OPTIONS=verbosity=1:log_threads=1
export LSAN_OPTIONS=suppressions="${SUPP_LSAN}"



#### run MusE binary file ##############
#
#  -p   Don't load LADSPA plugins
#  -S   Don't load MESS plugins
#  -N   Don't load LinuxVST plugins
#  -I   Don't load DSSI plugins
#  -2   Don't load LV2 plugins
#
#  -u   Ubuntu/unity workaround: don't allow sharing  menus and mdi-subwins.
#  -D   Debug mode: enable some debug messages
#          specify twice for lots of debug messages this
#          may slow down MusE massively!
#
#  -j  Use JAckAudio driver to connect to Jack audio
#  -J  Do not try to auto-start the Jack audio
# -y   Enable Python control support
# -Y N - midi realtime prio (alsa midi ?)
# -P N - audio realtime prio (set by jackd) 
# -R  Force plugin cache re-creation. 
# 
#  -D for DEBUG !
# 
/usr/local/bin/muse4     -j -J  -Y 8   --no-plugin-duplicate-warnings #  > error.log 2>&1 
#
########################################

