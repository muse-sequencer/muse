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

# NOTE: locale/ directory needs to be at the end so that the translations
#       are scanned before coming to share/locale

ADD_SUBDIRECTORY(drummaps)
ADD_SUBDIRECTORY(instruments)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(pybridge)
ADD_SUBDIRECTORY(scripts)
ADD_SUBDIRECTORY(templates)
ADD_SUBDIRECTORY(wallpapers)
ADD_SUBDIRECTORY(scoreglyphs)
ADD_SUBDIRECTORY(note_names)
ADD_SUBDIRECTORY(locale)
ADD_SUBDIRECTORY(metronome)
ADD_SUBDIRECTORY(rdf)

install (FILES didyouknow.txt splash.jpg DESTINATION ${MusE_SHARE_DIR} )

# themes/ is a plain data directory (Qlementine .json chrome themes at its
#  root, plus the old_themes/ and muse_custom/ subfolders) - no build logic
#  of its own, so it doesn't need (and doesn't have) its own CMakeLists.txt.
#  install(DIRECTORY ...) copies the whole tree recursively, subfolders
#  included, in one go - this is what ADD_SUBDIRECTORY(themes) used to do
#  via a themes/CMakeLists.txt, which no longer exists.
install (DIRECTORY themes/
      DESTINATION ${MusE_SHARE_DIR}/themes
      FILES_MATCHING
      PATTERN "*.json"
      PATTERN "*.qss"
      PATTERN "*.cfc"
      )
