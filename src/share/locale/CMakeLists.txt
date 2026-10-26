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

file(GLOB TRANSLATIONS_FILES *.ts)

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
endif (PYTHON_SUPPORT)

if(Qt6_VERSION VERSION_GREATER_EQUAL "6.7.0")

    # SINCE Qt 6.7:
    # 1. Let Qt 6 handle everything automatically.
    # This should not cause any errors about the command missing in Qt 6.2 - Qt 6.5.
    # It's not validated if it's skipped.
    qt_add_translations(muse
        # Fixes a CMake error
        IMMEDIATE_CALL
        # Explicitly listing .ts files is highly recommended over GLOB to prevent CMake sync issues.
        # But we we want users to be able to contribute ts files and we just drop them in without further work.
        TS_FILES  ${TRANSLATIONS_FILES}
            # locale/muse_cs.ts
            # locale/muse_de.ts
            # locale/muse_en.ts
            # locale/muse_es.ts
            # locale/muse_fr.ts
            # locale/muse_it.ts
            # locale/muse_pl.ts
            # locale/muse_ru.ts
        # Prevent future Git conflict churn. No line numbers.
        LUPDATE_OPTIONS "-locations" "none" "-no-sort"
        # This outputs the actual compiled .qm file paths.
        QM_FILES_OUTPUT_VARIABLE QM_FILES
        # Tell Qt to look for strings inside all sub-modules.
        SOURCE_TARGETS ${SRC_TARGETS}

        # Additional source files.
        SOURCES  ${FILES_TO_TRANSLATE}
    )

else(Qt6_VERSION VERSION_GREATER_EQUAL "6.7.0")

    # SINCE Qt 6.2 (Deprecated)
    # 1. Handle lupdate (The code scanner)
    qt_add_lupdate(
        TS_FILES ${TRANSLATIONS_FILES}
        # Git conflict prevention flags.
        OPTIONS "-locations" "none" "-no-sort"
        # Sweeps the loose source files.
        # There is no 'SOURCE_TARGETS' because it's not required in these commands.
        SOURCES ${FILES_TO_TRANSLATE}
    )

    # 2. Handle lrelease (The binary compiler)
    # This processes the files and directly populates your output variable without timing bugs
    qt_add_lrelease(
        TS_FILES ${TRANSLATIONS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES
    )

endif(Qt6_VERSION VERSION_GREATER_EQUAL "6.7.0")


install(FILES ${QM_FILES}
      DESTINATION ${MusE_SHARE_DIR}/locale
      )
