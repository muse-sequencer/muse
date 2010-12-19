#
#
#
#

include(${PROJECT_SOURCE_DIR}/pch.txt)

set (al_src
   al.cpp sig.cpp xml.cpp dsp.cpp 
   )

if (USE_SSE)
      set(al_src ${al_src} dspSSE.cpp dspXMM.cpp)
endif (USE_SSE)

add_library(al SHARED
   ${al_src}
   ${PROJECT_BINARY_DIR}/all.h.pch
   )

install(TARGETS al DESTINATION ${MUSE_MODULES_DIR})

target_link_libraries ( al
      ${QT_LIBRARIES}
      )

set_target_properties( al
      PROPERTIES OUTPUT_NAME muse_al
      )

set_source_files_properties(
   al.cpp sig.cpp xml.cpp dsp.cpp 
   dspXMM.cpp
   PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
   )

set_source_files_properties( dspSSE.cpp
   PROPERTIES COMPILE_FLAGS "-x assembler"
   )

