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
      jack.cpp dummyaudio.cpp
      )
ENDIF (APPLE)

add_library ( driver STATIC ${driver_src} )
set_target_properties( driver
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
   )

