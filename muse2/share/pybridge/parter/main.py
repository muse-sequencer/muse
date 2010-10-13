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

