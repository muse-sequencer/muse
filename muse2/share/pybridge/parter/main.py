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

import sys,time
from PyQt4 import QtGui

from parter import ParterMainwidget
import sys, os
import Pyro.core

#import musemock
#muse = musemock.MusEMock()
muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
"""
strack = muse.getSelectedTrack()
cpos = muse.getCPos()
muse.importPart(strack, "/home/ddskmlg/.muse/parts/testpart2.mpt", cpos)
sys.exit(0)
"""


if __name__ == '__main__':
      app = QtGui.QApplication(sys.argv)
      partsdir = os.getenv("HOME") + "/.muse/parts"
      mainw = ParterMainwidget(None, muse, partsdir)
      mainw.show()
      #muse.importPart("Track 1","/home/ddskmlg/.muse/parts/testpart2.mpt",18432)
      sys.exit(app.exec_())

