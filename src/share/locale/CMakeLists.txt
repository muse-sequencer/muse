#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

file (GLOB TRANSLATIONS_FILES *.ts)

if (UPDATE_TRANSLATIONS)
      qt5_create_translation(QM_FILES
          ${FILES_TO_TRANSLATE}
          ${TRANSLATIONS_FILES}
          OPTIONS -locations none -no-sort)
else ()
      QT5_ADD_TRANSLATION(QM_FILES ${TRANSLATIONS_FILES})
endif ()

add_custom_target (translations ALL DEPENDS ${QM_FILES})

set (SRC_TARGETS
            muse
            arranger
            cliplist
            ctrl
            driver
            instruments
            liste
            marker
            master
            midiedit
            mixer
            mplugins
            waveedit
            widgets
            awl
            deicsonze
            fluid_synth
            synti
            organ
            simpledrums
            vam
            components
            plugin_scan_module
            grepmidi
)

if (PYTHON_SUPPORT)
      set ( SRC_TARGETS ${SRC_TARGETS} remote )
endif ()


# If we are updating the translations, make sure that these targets
# are scanned first
if (UPDATE_TRANSLATIONS)
      add_dependencies(translations ${SRC_TARGETS})
endif ()

install(FILES ${QM_FILES}
      DESTINATION ${MusE_SHARE_DIR}/locale
      )
