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
muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')

#
# Example on how to insert a new note, outcommented since I run the script several times and it inserts so many notes :-)
# But it works!
#


rpos = muse.getRPos()
lpos = muse.getLPos()

event = {'data':[61,100,0],
      'tick':0, # Relative offset of part - 0 = beginning of part
      'type':"note",
      'len':rpos - lpos}

part = {'events': [event],
         'tick': lpos}
muse.createPart("Track 1", lpos, rpos - lpos, part)

