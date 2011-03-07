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

for j in range(0,5):
      for i in range(0,30):
            muse.addMidiTrack("amiditrack" + str(i))
      for i in range(0,30):
            muse.deleteTrack("amiditrack" + str(i))

for i in range(0, 10):
      print i
      muse.addMidiTrack("amiditrack")
      muse.addWaveTrack("awavetrack")
      muse.addOutput("anoutput")
      muse.addInput("aninput")
      muse.setMute("aninput", False)
      muse.setAudioTrackVolume("aninput",1.0)
      muse.deleteTrack("amiditrack")
      muse.deleteTrack("awavetrack")
      muse.deleteTrack("anoutput")
      muse.deleteTrack("aninput")
      time.sleep(1)

