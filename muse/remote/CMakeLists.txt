#
#
#
#

include(${PROJECT_SOURCE_DIR}/pch.txt)

file (GLOB remote_src
   pyapi.cpp
   )

add_library(remote STATIC
   ${remote_src}
   ${PROJECT_BINARY_DIR}/all.h.pch
   )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${remote_src} CACHE INTERNAL "")

set_source_files_properties(
   pyapi.cpp
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h -include ${PYTHON_INCLUDES} -DENABLE_PYTHON"
   )

