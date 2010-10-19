#
#
#
#

include(${PROJECT_SOURCE_DIR}/pch.txt)

set (al_src
   al.cpp dsp.cpp
   )

if (USE_SSE)
      set(al_src ${al_src} dspSSE.cpp dspXMM.cpp)
endif (USE_SSE)

add_library(al STATIC
   ${al_src}
   ${PROJECT_BINARY_DIR}/all.h.pch
   )

set_source_files_properties(
   al.cpp dsp.cpp
   dspXMM.cpp
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
   )

set_source_files_properties( dspSSE.cpp
   PROPERTIES COMPILE_FLAGS "-x assembler"
   )

