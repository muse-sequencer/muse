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

import sys,time,os
from PyQt5.QtWidgets import QWidget, QFileDialog, QListView, QTreeView, QDirModel, QApplication
from PyQt5.QtWidgets import QButtonGroup, QPushButton, QLabel, QGridLayout
from PyQt5.QtCore import QFileInfo

class ParterMainwidget(QWidget):
      def __init__(self, parent=None, muse=None, partsdir=None):
            QWidget.__init__(self, parent)
            self.muse = muse
            self.partsdir = partsdir
            self.lcurdir = QLabel(partsdir)
            moveupbutton = QPushButton("Parent dir")
            appendbutton = QPushButton("Append")
            putbutton = QPushButton("Put")
            blayout = QGridLayout()
            blayout.addWidget(self.lcurdir)
            blayout.addWidget(moveupbutton)
            blayout.addWidget(appendbutton)
            blayout.addWidget(putbutton)
            self.tree = QTreeView()
            self.dirmodel = QDirModel()
            self.tree.setModel(self.dirmodel)
            self.tree.setRootIndex(self.dirmodel.index(self.partsdir))

            layout = QGridLayout()
            self.setLayout(layout)
            layout.addWidget(self.tree, 0, 0)
            layout.addLayout(blayout, 0, 1)

            moveupbutton.clicked.connect(self.parentDir) 
            appendbutton.clicked.connect(self.appendPressed) 
            putbutton.clicked.connect(self.putPressed) 

            self.tree.doubleClicked.connect(self.activated)

      def parentDir(self):
            f = QFileInfo(self.partsdir)
            self.changeDir(f.canonicalPath())

      def changeDir(self, newdir):
            self.partsdir = newdir
            self.tree.setRootIndex(self.dirmodel.index(self.partsdir))
            self.lcurdir.setText(self.partsdir)

      def activated(self, s):
            fileInfo = self.dirmodel.fileInfo(s)
            if fileInfo.isDir():
                  self.changeDir(fileInfo.absoluteFilePath())
                  return

            fname = str(fileInfo.absoluteFilePath()) # if not str() around it crashes!
            self.putPart(fname)

      def putPart(self, fname):
            trackid = self.muse.getSelectedTrack()
            if trackid == None:
                  return
            cpos = self.muse.getCPos()
            self.muse.importPart(trackid, fname, cpos)

      def getSelectedItem(self):
            selectionmodel = self.tree.selectionModel()
            for i in selectionmodel.selectedIndexes():
                  fileInfo = self.dirmodel.fileInfo(i)
                  return str(fileInfo.absoluteFilePath())
            return None

      def appendPressed(self):
            selected = self.getSelectedItem()
            if selected == None:
                  return
            trackid = self.muse.getSelectedTrack()
            if trackid == None:
                  return
            parts = self.muse.getParts(trackid)
            if parts == None:
                  return

            pos = 0
            if len(parts) > 0:
                  part = parts[len(parts) - 1]
                  pos = part['tick'] + part['len']
            print ("Appending " + selected)
            self.muse.importPart(trackid, selected, pos)

                  

      def putPressed(self):
            selected = self.getSelectedItem()
            if selected == None:
                  return
            trackid = self.muse.getSelectedTrack()
            if trackid == None:
                  return
            cpos = self.muse.getCPos()
            self.muse.importPart(trackid, selected, cpos)

      def testfunc2(self, index):
            print (str(index.row()) + " " + str(index.column()))
            print (index.data().toString())

if __name__ == '__main__':
      app = QApplication(sys.argv)
      mainw = ParterMainwidget()
      mainw.show()
      sys.exit(app.exec_())


