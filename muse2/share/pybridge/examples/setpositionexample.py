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
parts = muse.getParts("Track 1")

ptick = parts[0]['tick']
len = parts[0]['len']
muse.setLPos(ptick)
muse.setRPos(ptick + len)
muse.setCPos(ptick + len / 2)

songlen = muse.getSongLen()
#print "Song length: " + str(songlen)

#
# Copy first part to after current song length, thus increase song length with length of first part
#
newsonglen = songlen + parts[0]['len']
muse.setSongLen(newsonglen)
muse.createPart("Track 1", songlen + 1, parts[0]['len'], parts[0])
time.sleep(1)

lastpart = muse.getParts("Track 1").pop()
print lastpart['id']
muse.deletePart(lastpart['id'])
print muse.getDivision()

