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

muse.setMidiTrackParameter("Track 1", "velocity",10)
muse.setMidiTrackParameter("Track 1", "compression",101)
muse.setMidiTrackParameter("Track 1", "delay",2)
muse.setMidiTrackParameter("Track 1", "transposition",1)

for i in range(-127, 127):
      muse.setMidiTrackParameter("Track 1", "velocity",i)
      time.sleep(0.1)

