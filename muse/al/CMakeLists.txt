#
#
#
#

add_library(al STATIC
   al.cpp marker.cpp pos.cpp sig.cpp tempo.cpp xml.cpp
   )
set_target_properties( al
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_SOURCE_DIR}/all.h"
   )

add_dependencies(al ${PROJECT_SOURCE_DIR}/all.h.pch)

