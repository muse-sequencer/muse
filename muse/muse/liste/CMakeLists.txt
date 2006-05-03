#
#
#
#

MACRO(WRAP_CPP outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.cpp)
      ENDFOREACH(it)
   ENDMACRO(WRAP_CPP)

MACRO(WRAP_H outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.h)
      ENDFOREACH(it)
   ENDMACRO(WRAP_H)

MACRO(WRAP_UI outfiles )
   FOREACH(it ${ARGN})
      SET(${outfiles} ${${outfiles}} ${it}.ui)
      ENDFOREACH(it)
   ENDMACRO(WRAP_UI)

set(mocs
      listedit editevent
      )

WRAP_H(mocH ${mocs})
WRAP_CPP(mocCPP ${mocs})

QT4_WRAP_CPP (moc_headers ${mocH})

set (uics
      editnotedialog
      )

WRAP_UI(uiUI ${uics})
QT4_WRAP_UI (ui_headers ${uiUI})

set ( liste_src ${mocCPP} ${moc_headers})

add_library ( liste STATIC ${liste_src})
set_target_properties( liste
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/all.h"
   )

