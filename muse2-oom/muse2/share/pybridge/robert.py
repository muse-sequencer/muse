# -*- coding: utf-8 -*-
#
# Example client for MusE Pyro bridge (Python Remote Object)
#
import Pyro.core
import time

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
print "Current position is: " + str(muse.getCPos())

midiDevice=file("/dev/snd/midiC1D0")
nextIsCommand=False
while True:
  v=midiDevice.read(1)
  if nextIsCommand:
    print "   %d"%ord(v)
    if ord(v) == 0:
	print "set hh"
        muse.setMute("hh", False)
        muse.setMute("RIDE", True)
    if ord(v) == 1:
        muse.setMute("hh", True)
        muse.setMute("RIDE", False)
	print "set ride"
    if ord(v) == 2:
        muse.setMute("ACCENT1", False)
    if ord(v) == 3:
        muse.setMute("ACCENT2", False)
    if ord(v) == 127:
	print "mute all accents"
        muse.setMute("ACCENT1", True)
        muse.setMute("ACCENT2", True)
    nextIsCommand=False
  if ord(v) == 192:
     nextIsCommand=True

'''
muse.startPlay()
time.sleep(1) # Sleep one second
muse.stopPlay()
print "New position is: " + str(muse.getCPos())
muse.rewindStart()
print "Pos after rewind is: " + str(muse.getCPos())
print "Lpos, Rpos: " + str(muse.getLPos()) + ":" + str(muse.getRPos())

'''
