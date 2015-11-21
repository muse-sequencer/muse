
cmake_policy(SET CMP0005 NEW)
##
## List of source files to compile
##
file (GLOB lv2_support_files
lv2/lv2plug.in/ns/ext/uri-map/uri-map.h
lv2/lv2plug.in/ns/ext/parameters/parameters.h
lv2/lv2plug.in/ns/ext/buf-size/buf-size.h
lv2/lv2plug.in/ns/ext/instance-access/instance-access.h
lv2/lv2plug.in/ns/ext/time/time.h
lv2/lv2plug.in/ns/ext/event/event.h
lv2/lv2plug.in/ns/ext/event/event-helpers.h
lv2/lv2plug.in/ns/ext/presets/presets.h
lv2/lv2plug.in/ns/ext/resize-port/resize-port.h
lv2/lv2plug.in/ns/ext/patch/patch.h
lv2/lv2plug.in/ns/ext/options/options.h
lv2/lv2plug.in/ns/ext/data-access/data-access.h
lv2/lv2plug.in/ns/ext/urid/urid.h
lv2/lv2plug.in/ns/ext/port-props/port-props.h
lv2/lv2plug.in/ns/ext/state/state.h
lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h
lv2/lv2plug.in/ns/ext/morph/morph.h
lv2/lv2plug.in/ns/ext/midi/midi.h
lv2/lv2plug.in/ns/ext/worker/worker.h
lv2/lv2plug.in/ns/ext/port-groups/port-groups.h
lv2/lv2plug.in/ns/ext/log/logger.h
lv2/lv2plug.in/ns/ext/log/log.h
lv2/lv2plug.in/ns/ext/atom/atom-test.c
lv2/lv2plug.in/ns/ext/atom/atom.h
lv2/lv2plug.in/ns/ext/atom/forge.h
lv2/lv2plug.in/ns/ext/atom/util.h
lv2/lv2plug.in/ns/extensions/units/units.h
lv2/lv2plug.in/ns/extensions/ui/ui.h
lv2/lv2plug.in/ns/lv2core/lv2.h
sratom/sratom.h
sratom/sratom.c
sord/sord.h
sord/sord.c
sord/syntax.c
sord/sord_internal.h
serd/writer.c
serd/reader.c
serd/uri.c
serd/serd_internal.h
serd/env.c
serd/serd.h
serd/string.c
serd/node.c
lilv/world.c
lilv/collections.c
lilv/scalepoint.c
lilv/lilv_internal.h
lilv/lib.c
lilv/query.c
lilv/lilv.h
lilv/port.c
lilv/ui.c
lilv/state.c
lilv/plugin.c
lilv/instance.c
lilv/node.c
lilv/pluginclass.c
lilv/util.c
zix/common.h
zix/digest.c
zix/hash.h
zix/digest.h
zix/tree.h
zix/hash.c
zix/tree.c
lv2extui.h
lv2extprg.h
      )
set(CMAKE_C_FLAGS "-std=c99")


if (PCRE_FOUND)
      include_directories(${PCRE_INCLUDE_DIRS})
      add_definitions(-DHAVE_PCRE=1)
endif (PCRE_FOUND)

CHECK_INCLUDE_FILES (sys/file.h HAVE_FILE_H)

if(HAVE_FILE_H)
# Check for jack session availability
check_c_source_compiles (
    "#include <sys/file.h>
     #include <stdio.h>
     #define _POSIX_C_SOURCE=1
    int main(int argc, char **argv)
    {
        FILE *f = 0;
        flock(0, 0);
        fileno(f);
        return 0;
    }" HasFileNo
)
    if (HasFileNo)
        add_definitions(-DHAVE_FLOCK=1)
        add_definitions(-DHAVE_FILENO=1)
    endif (HasFileNo)
endif(HAVE_FILE_H)

set(CMAKE_C_FLAGS "-Wall -fPIC ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -Werror -Wall -fPIC ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g0 -Wall -fPIC ${CMAKE_C_FLAGS_RELEASE}")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
add_definitions(-DHAVE_LV2=1)
add_definitions(-DHAVE_SERD=1)
add_definitions(-DHAVE_SORD=1)
add_definitions(-DLILV_VERSION="0.20.0")
add_definitions(-DLILV_NEW_LV2=1)
add_definitions(-DLILV_PATH_SEP=":")
add_definitions(-DLILV_DIR_SEP="/")
add_definitions(-DLILV_INTERNAL=1)
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DSERD_INTERNAL=1)
add_definitions(-DSORD_INTERNAL=1)
add_definitions(-DSRATOM_INTERNAL=1)
Set (LILV_DEFAULT_LV2_PATH ~/.lv2:/usr/lib${LIB_SUFFIX}/lv2:/usr/local/lib${LIB_SUFFIX}/lv2)
add_definitions(-DLILV_DEFAULT_LV2_PATH="${LILV_DEFAULT_LV2_PATH}")

message (STATUS "LILV_DEFAULT_LV2_PATH= ${LILV_DEFAULT_LV2_PATH}")

##
## Define target
##
add_library ( lv2_support STATIC ${lv2_support_files} )
