#
#
#
#

add_library(al STATIC
   al.cpp marker.cpp pos.cpp sig.cpp tempo.cpp xml.cpp
   dsp.cpp
   dspSSE.cpp
   dspXMM.cpp
   )

set_source_files_properties(
   al.cpp marker.cpp pos.cpp sig.cpp tempo.cpp xml.cpp
   dsp.cpp
   dspXMM.cpp
   PROPERTIES COMPILE_FLAGS "-msse2 -fPIC -include ${PROJECT_SOURCE_DIR}/all.h"
   )


set_source_files_properties( dspSSE.cpp
   PROPERTIES COMPILE_FLAGS "-x assembler"
   )

add_dependencies(al ${PROJECT_SOURCE_DIR}/all.h.pch)

