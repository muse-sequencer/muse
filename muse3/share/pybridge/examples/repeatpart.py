"""
//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
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
//=========================================================
"""

import Pyro.core
import sys
import time

SLEEPIVAL=0.3

def advanceToNextSection(muse, newlpos, newrpos):
      print "Advancing..."
      currpos = muse.getRPos()
      curlpos = muse.getLPos()
      curpos = muse.getCPos()
      muse.setLoop(False)

      while curpos < currpos:
            time.sleep(SLEEPIVAL)
            curpos = muse.getCPos()
      print "Leaving current section..."
      muse.setRPos(newrpos)
      curpos = muse.getCPos()

      while curpos < newlpos:
            time.sleep(SLEEPIVAL)
            curpos = muse.getCPos()
      print "Entered new section"
      muse.setLPos(newlpos)
      muse.setLoop(True)
      return

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
muse.stopPlay()
parts = muse.getParts("Track 1")
muse.setLPos(parts[0]['tick'])
muse.setRPos(parts[0]['tick'] + parts[0]['len'])
muse.setCPos(0)
time.sleep(0.2) # Hmmm, don't like it but it seems necessary to pause a short while before starting play
muse.setLoop(True)
muse.startPlay()

for i in range(1, len(parts)):
      part = parts[i]
      tick = part['tick']
      len = part['len']
      print "Press enter to advance to next section/part!"
      sys.stdin.read(1)
      advanceToNextSection(muse, tick, tick + len)

print "This is the final section. Disabling loop and leaving..."
muse.setLoop(False)

#print "Press enter to leave final section"
#sys.stdin.read(1)
#muse.setLoop(False)

