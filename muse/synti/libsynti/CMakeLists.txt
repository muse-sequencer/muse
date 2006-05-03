#
#
#
#

add_library(synti mess.cpp gui.cpp mono.cpp poly.cpp mpevent.cpp)
set_target_properties( synti
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
   )

