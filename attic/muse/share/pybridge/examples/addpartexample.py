"""
//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
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

