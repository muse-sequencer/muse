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

#
# produce the .desktop file
#
configure_file (
      muse.desktop.in
      ${PROJECT_BINARY_DIR}/packaging/muse.desktop
      )

#
# install the desktop and mime files
#
install( FILES muse_icon.png
      DESTINATION ${SHARE_INSTALL_PREFIX}/icons/hicolor/64x64/apps/
      )
install( FILES ${PROJECT_BINARY_DIR}/packaging/muse.desktop
      DESTINATION ${SHARE_INSTALL_PREFIX}/applications/
      )
install( FILES muse.xml
      DESTINATION ${SHARE_INSTALL_PREFIX}/mime/packages/
      )

