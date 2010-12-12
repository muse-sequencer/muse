#
#
#
#

# MACRO(WRAP_CPP outfiles )
#    FOREACH(it ${ARGN})
#       SET(${outfiles} ${${outfiles}} ${it}.cpp)
#       ENDFOREACH(it)
#    ENDMACRO(WRAP_CPP)
# 
# MACRO(WRAP_H outfiles )
#    FOREACH(it ${ARGN})
#       SET(${outfiles} ${${outfiles}} ${it}.h)
#       ENDFOREACH(it)
#    ENDMACRO(WRAP_H)
# 
# MACRO(WRAP_UI outfiles )
#    FOREACH(it ${ARGN})
#       SET(${outfiles} ${${outfiles}} ${it}.ui)
#       ENDFOREACH(it)
#    ENDMACRO(WRAP_UI)
# 
# set(mocs
#       markerview
#       )
# 
# WRAP_H(mocH ${mocs})
# WRAP_CPP(mocCPP ${mocs})
# 
# QT4_WRAP_CPP (moc_headers ${mocH})
# 
# set (uics
#    )
# 
# WRAP_UI(uiUI ${uics})
# QT4_WRAP_UI (ui_headers ${uiUI})
# 
# set ( marker_src ${mocCPP} ${moc_headers})
# 
# add_library ( marker STATIC ${marker_src})
# set_target_properties( marker
#    PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
#    )



QT4_WRAP_CPP ( marker_mocs
        markerview.h
      )

file (GLOB SOURCE_FILES
        marker.cpp marker.h 
        markerview.cpp # markerview.h
        )

add_library ( marker STATIC
        ${SOURCE_FILES}
        # ${marker_ui_headers}
        ${marker_mocs}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} CACHE INTERNAL "")

set_target_properties( marker
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      )

