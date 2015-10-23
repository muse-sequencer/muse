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
# Example client for MusE Pyro bridge (Python Remote Object)
#
import Pyro.core
import time

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
print "Current position is: " + str(muse.getCPos())
muse.startPlay()
time.sleep(1) # Sleep one second
muse.stopPlay()
print "New position is: " + str(muse.getCPos())
muse.rewindStart()
print "Pos after rewind is: " + str(muse.getCPos())
print "Lpos, Rpos: " + str(muse.getLPos()) + ":" + str(muse.getRPos())


