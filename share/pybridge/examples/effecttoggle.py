"""
//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================
"""

import Pyro.core
import time

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
trackname = "wavtrack"

fxs = muse.getTrackEffects(trackname)
print fxs

for i in range (0,10):
      muse.toggleTrackEffect(trackname,0, False)
      time.sleep(1)
      muse.toggleTrackEffect(trackname,0, True)
      time.sleep(1)

