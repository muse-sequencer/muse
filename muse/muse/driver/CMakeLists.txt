#
#
#
#

IF (APPLE)
set ( driver_src 
      jack.cpp dummyaudio.cpp coretimer.cpp coremidi.cpp
      )
ELSE (APPLE)
set ( driver_src 
     alsamidi.cpp jack.cpp dummyaudio.cpp alsatimer.cpp
      rtctimer.cpp posixtimer.cpp
      )
ENDIF (APPLE)

add_library ( driver STATIC ${driver_src} )
set_target_properties( driver
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
   )

