
include_directories(${GTK2_INCLUDE_DIRS})

##
## List of source files to compile
##
file (GLOB lv2_gtk2_support_source_files
      lv2Gtk2Support.cpp
      )

##
## Define target
##
add_library ( lv2_gtk2_support ${MODULES_BUILD}
      ${lv2_gtk2_support_source_files}
      )

##
## Append to the list of translations
##
# set (FILES_TO_TRANSLATE
#       ${FILES_TO_TRANSLATE}
#       ${lv2_gtk2_support_source_files}
#       CACHE INTERNAL ""
#       )

##
## Compilation flags and target name
##
set_target_properties( lv2_gtk2_support
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME muse_lv2_gtk2_support
      )

##
## Linkage
##
target_link_libraries ( lv2_gtk2_support
      ${GTK2_LIBRARIES}
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS lv2_gtk2_support
            DESTINATION ${MusE_MODULES_DIR}
            )
else ( ${MODULES_BUILD} STREQUAL SHARED )
      target_link_libraries (lv2_gtk2_support core)
endif ( ${MODULES_BUILD} STREQUAL SHARED )
