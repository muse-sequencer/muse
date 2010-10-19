#
#
#
#

include(${PROJECT_SOURCE_DIR}/pch.txt)

set (remote_src
   pyapi.cpp
   )

add_library(remote STATIC
   ${remote_src}
   ${PROJECT_BINARY_DIR}/all.h.pch
   )

set_source_files_properties(
   pyapi.cpp
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h -include ${PYTHON_INCLUDES} -DENABLE_PYTHON"
   )

