#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

