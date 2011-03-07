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

muse.setMidiTrackParameter("Track 1", "velocity",10)
muse.setMidiTrackParameter("Track 1", "compression",101)
muse.setMidiTrackParameter("Track 1", "delay",2)
muse.setMidiTrackParameter("Track 1", "transposition",1)

for i in range(-127, 127):
      muse.setMidiTrackParameter("Track 1", "velocity",i)
      time.sleep(0.1)

