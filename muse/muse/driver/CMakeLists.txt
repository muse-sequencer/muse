#
#
#
#

set ( driver_src 
      alsamidi.cpp jack.cpp dummyaudio.cpp alsatimer.cpp
      rtctimer.cpp posixtimer.cpp
      )

add_library ( driver STATIC ${driver_src} )
set_target_properties( driver
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
   )

