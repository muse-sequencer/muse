#!/bin/bash

### This script starts muse with various debug options, only for developers. 



SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
## echo "This bash script is located in: $SCRIPT_DIR"


SUPP_VALG="${SCRIPT_DIR}/misc/valgrind_muse.supp"
SUPP_LSAN="${SCRIPT_DIR}/misc/lsan.supp"



### (optional) use virtual python environment , set QT paths; WARNING: can conflict with QT5 linked by muse
# if [ -n "$VIRTUAL_ENV" ]; then
#     PYVER=$(python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
#     echo "INFO: Using python virtual_env : ${PYVER}"
# 
#     export QT_PLUGIN_PATH="$VIRTUAL_ENV/lib/python$PYVER/site-packages/PyQt6/Qt6/plugins:/usr/lib/qt6/plugins"
#     export LD_LIBRARY_PATH="$VIRTUAL_ENV/lib/python$PYVER/site-packages/PyQt6/Qt6/lib"
# 
# else
#     echo "INFO: Not using python virtual_env."
# fi



export QT_QPA_PLATFORM=xcb
export GDK_BACKEND=x11
export SDL_VIDEODRIVER=x11
export CLUTTER_BACKEND=x11


# HINT: For debugging, try setting LSAN (leak sanitizer) environment variable: 
#export LSAN_OPTIONS=verbosity=1:log_threads=1
##### optional, use lsan suppression file 
export LSAN_OPTIONS=suppressions="${SUPP_LSAN}"


### optional preload
#export LD_PRELOAD=/usr/lib/libasan.so:/usr/local/lib/libinstpatch-1.0.so 


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
# -y Enable Python control support
# -Y N - midi realtime prio (alsa midi ?)
# -P N - audio realtime prio (set by jackd) 
# -R  Force plugin cache re-creation. 
# 
#  -D for DEBUG !
/usr/local/bin/muse5  -D -D  -j -J  -Y 8   --no-plugin-duplicate-warnings  > error.log 2>&1 
#
########################################



###### DEBUG methods ##################

## --leak-check=summary  OR  --leak-check=full
##  --show-error-list=no --trace-children=no  
## suppress summary: --quiet

##  reduce output 
## --errors-for-leak-kinds=definite
## --errors-for-leak-kinds=all


#### very long output !  ~11 MB
# valgrind \
#   --leak-check=full \
#   --show-leak-kinds=all \
#   --error-limit=yes \
#   --track-origins=yes \
#   --gen-suppressions=all \
#   --num-callers=11 \
#   --suppressions="${SUPP_VALG}" \
#   /usr/local/bin/muse4  2> valg.out.txt


###  only mem leaks  - USEFUL
# valgrind --leak-check=full --show-leak-kinds=definite --track-origins=yes  \
#    --gen-suppressions=all \
#    --suppressions="${SUPP_VALG}" \
#    /usr/local/bin/muse4  2> valg.out.txt



#gdb --args /usr/local/bin/muse4  -j -J  -Y 8   --no-plugin-duplicate-warnings  #2> error.log
#
# HINT: LeakSanitizer does not work under ptrace (strace, gdb, etc)


