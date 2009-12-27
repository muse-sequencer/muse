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

