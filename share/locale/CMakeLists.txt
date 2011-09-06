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
      qt4_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})
else (UPDATE_TRANSLATIONS)
      qt4_add_translation(QM_FILES ${TRANSLATIONS_FILES})
endif (UPDATE_TRANSLATIONS)

add_custom_target (translations ALL DEPENDS ${QM_FILES})

# If we are updating the translations, make sure that these targets
# are scanned first
if (UPDATE_TRANSLATIONS)
      add_dependencies(translations
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
            remote
            waveedit
            widgets
            awl
            deicsonze
            fluid
            fluidsynth
            libsynti
            organ
            s1
            simpledrums2
            vam
            )
endif (UPDATE_TRANSLATIONS)

install(FILES ${QM_FILES}
      DESTINATION ${MusE_SHARE_DIR}/locale
      )
