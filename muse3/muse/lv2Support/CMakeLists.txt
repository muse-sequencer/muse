
cmake_policy(SET CMP0005 NEW)

include_directories(${muse_sord_dir}/src)
include_directories(${muse_lilv_dir}/src)

##
## List of source files to compile
##
file (GLOB lv2_support_files
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/uri-map/uri-map.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/parameters/parameters.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/buf-size/buf-size.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/instance-access/instance-access.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/time/time.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/event/event.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/event/event-helpers.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/presets/presets.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/resize-port/resize-port.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/patch/patch.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/options/options.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/data-access/data-access.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/urid/urid.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/port-props/port-props.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/state/state.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/morph/morph.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/midi/midi.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/worker/worker.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/port-groups/port-groups.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/log/logger.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/log/log.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/atom/atom-test.c
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/atom/atom.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/atom/forge.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/ext/atom/util.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/extensions/units/units.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/extensions/ui/ui.h
${muse_lv2_dir}/lv2/lv2plug.in/ns/lv2core/lv2.h

${muse_sratom_dir}/sratom/sratom.h
${muse_sratom_dir}/src/sratom.c

${muse_sord_dir}/sord/sord.h
${muse_sord_dir}/src/sord.c
${muse_sord_dir}/src/syntax.c
${muse_sord_dir}/src/sord_internal.h

${muse_serd_dir}/src/writer.c
${muse_serd_dir}/src/reader.c
${muse_serd_dir}/src/uri.c
${muse_serd_dir}/src/serd_internal.h
${muse_serd_dir}/src/env.c
${muse_serd_dir}/serd/serd.h
${muse_serd_dir}/src/string.c
${muse_serd_dir}/src/node.c

${muse_lilv_dir}/src/world.c
${muse_lilv_dir}/src/collections.c
${muse_lilv_dir}/src/scalepoint.c
${muse_lilv_dir}/src/lilv_internal.h
${muse_lilv_dir}/src/lib.c
${muse_lilv_dir}/src/query.c
${muse_lilv_dir}/lilv/lilv.h
${muse_lilv_dir}/src/port.c
${muse_lilv_dir}/src/ui.c
${muse_lilv_dir}/src/state.c
${muse_lilv_dir}/src/plugin.c
${muse_lilv_dir}/src/instance.c
${muse_lilv_dir}/src/node.c
${muse_lilv_dir}/src/pluginclass.c
${muse_lilv_dir}/src/util.c

${muse_sord_dir}/src/zix/common.h
${muse_sord_dir}/src/zix/digest.c
${muse_sord_dir}/src/zix/hash.h
${muse_sord_dir}/src/zix/digest.h
${muse_sord_dir}/src/zix/btree.h
${muse_sord_dir}/src/zix/hash.c
${muse_sord_dir}/src/zix/btree.c
${muse_lilv_dir}/src/zix/tree.h
${muse_lilv_dir}/src/zix/tree.c

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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -Werror -Wall -fPIC")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g0 -Wall -fPIC")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
add_definitions(-DHAVE_LV2=1)
add_definitions(-DHAVE_SERD=1)
add_definitions(-DHAVE_SORD=1)
add_definitions(-DLILV_VERSION=${muse_lilv_ver})
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

# Complains if C not set.
set_target_properties( lv2_support
      PROPERTIES LINKER_LANGUAGE C
      )

