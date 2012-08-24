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
import time

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
#for i in range(0,10):
#      print "Ctrl no " + str(i) + " = " + str(muse.getMidiControllerValue("Track 1", i))

"""
for i in range(0,127):
      muse.setMidiControllerValue("Track 1", 7, i)
      time.sleep(0.1)
"""

muse.setMidiControllerValue("Track 1", 7, 56)
print muse.getMidiControllerValue("Track 1", 7)
print muse.getAudioTrackVolume("Out 1")
muse.setAudioTrackVolume("Out 1", -1.0)

