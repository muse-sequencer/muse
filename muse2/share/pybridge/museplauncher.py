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

This file is used by MusE for launching a Pyro name service and connecting a remote object to the global Python functions
"""

import Pyro.naming
import Pyro.core
from Pyro.errors import PyroError,NamingError
import sys, time
import threading

#
# Note: this module, 'muse' is activated from within MusE - thus it is not possible to execute the scripts without a running
# MusE instance
#
import muse 

#
# Class which implements the functionality that is used remotely. 
# In short just repeating the global functions in the muse-module
#
# TODO: It should be better to skip this class completely by implementing 
# functionality as a class in pyapi.cpp instead of global functions
# that need to be wrapped like this
#
class MusE:
      def getCPos(self): # Get current position
            return muse.getCPos()

      def startPlay(self): # Start playback
            return muse.startPlay()

      def stopPlay(self): # Stop playback
            return muse.stopPlay()

      def rewindStart(self): # Rewind current position to start
            return muse.rewindStart()

      def getLPos(self): # Get position of left locator
            return muse.getLPos()

      def getRPos(self): # Get position of right locator
            return muse.getRPos()

      def getTempo(self, tick): #Get tempo at particular tick
            return muse.getTempo(tick)

      def getTrackNames(self): # get track names
            return muse.getTrackNames()

      def getParts(self, trackname): # get parts in a particular track
            return muse.getParts(trackname)

      def createPart(self, trackname, starttick, lenticks, part): # create part in track
            return muse.createPart(trackname, starttick, lenticks, part)

      def modifyPart(self, part): # modify a part (the part to be modified is specified by its id
            return muse.modifyPart((part))

      def deletePart(self, part): # delete a part
            return muse.deletePart((part))

      def getSelectedTrack(self): # get first selected track in arranger window
            return muse.getSelectedTrack()

      def importPart(self, trackname, filename, tick): # import part file to a track at a given position
            return muse.importPart(trackname, filename, tick)

      def setCPos(self, tick): # set current position
            return muse.setPos(0, tick)

      def setLPos(self, tick): # set left locator
            return muse.setPos(1, tick)

      def setRPos(self, tick): # set right locator
            return muse.setPos(2, tick)
      
      def setSongLen(self, ticks): # set song length
            return muse.setSongLen(ticks)

      def getSongLen(self): # get song length
            return muse.getSongLen()

      def getDivision(self): # get division (ticks per 1/4, or per beat?)
            return muse.getDivision()

      def setMidiTrackParameter(self, trackname, paramname, value): # set midi track parameter (velocity, compression, len, transpose)
            return muse.setMidiTrackParameter(trackname, paramname, value);

      def getLoop(self): # get loop flag
            return muse.getLoop()

      def setLoop(self, loopFlag): # set loop flag
            return muse.setLoop(loopFlag)
      
      def getMute(self, trackname): # get track mute parameter
            return muse.getMute(trackname)

      def setMute(self, trackname, enabled): # set track mute parameter
            return muse.setMute(trackname, enabled)

      def setVolume(self, trackname, volume): # set mixer volume
            return muse.setVolume(trackname, volume)

      def getMidiControllerValue(self, trackname, ctrlno): # get a particular midi controller value for a track
            return muse.getMidiControllerValue(trackname, ctrlno)

      def setMidiControllerValue(self, trackname, ctrlno, value): # set a particular midi controller value for a track
            return muse.setMidiControllerValue(trackname, ctrlno, value)

      def setAudioTrackVolume(self, trackname, dvol): # set volume for audio track 
            return muse.setAudioTrackVolume(trackname, dvol)

      def getAudioTrackVolume(self, trackname): # get volume for audio track
            return muse.getAudioTrackVolume(trackname)

      def getTrackEffects(self, trackname): # get effect names for an audio track
            return muse.getTrackEffects(trackname)

      def toggleTrackEffect(self, trackname, effectno, onoff): # toggle specific effect on/off
            return muse.toggleTrackEffect(trackname, effectno, onoff)

      def findNewTrack(self, oldtracknames): #internal function
            tracknames = muse.getTrackNames()
            for trackname in tracknames:
                  if trackname in oldtracknames:
                        continue

                  return trackname

      def changeTrackName(self, trackname, newname): #change track name
            return muse.changeTrackName(trackname, newname)

      def nameNewTrack(self, newname, oldtracknames):# Internal function, wait until new track shows up in tracknames, then rename it
            tmpname = None
            for i in range(0,100):
                  tmpname = self.findNewTrack(oldtracknames)
                  if tmpname == None:
                        time.sleep(0.1)
                        continue
                  else:
                        self.changeTrackName(tmpname, newname)
                        time.sleep(0.1) # Ouch!!
                        break


      def addMidiTrack(self, trackname): # add midi track
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addMidiTrack()
            self.nameNewTrack(trackname, oldtracknames)
            

      def addWaveTrack(self, trackname): # add wave track
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addWaveTrack()
            self.nameNewTrack(trackname, oldtracknames)

      def addInput(self, trackname): # add audio input
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addInput()
            self.nameNewTrack(trackname, oldtracknames)

      def addOutput(self, trackname): # add audio output
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addOutput()
            self.nameNewTrack(trackname, oldtracknames)

      def addGroup(self, trackname): # add audio group
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addGroup()
            self.nameNewTrack(trackname, oldtracknames)

      def deleteTrack(self, trackname): # delete a track
            tracknames = muse.getTrackNames()
            if trackname not in tracknames:
                  return False

            muse.deleteTrack(trackname)

#      def getOutputRoute(self, trackname):
#            return muse.getOutputRoute(trackname)

class NameServiceThread(threading.Thread):
      def __init__(self):
            threading.Thread.__init__(self)
            self.starter = Pyro.naming.NameServerStarter()

      def run(self):
            self.starter.start()

      def verifyRunning(self):
            return self.starter.waitUntilStarted(10)

#
# museclass Pyro object
#
class museclass(Pyro.core.ObjBase, MusE):
      pass

#
# main server program
#
def main():
      Pyro.core.initServer()
      nsthread = NameServiceThread()
      nsthread.start()
      if (nsthread.verifyRunning() == False):
            print "Failed to launch name service..."
            sys.exit(1)

      daemon = Pyro.core.Daemon()
      # locate the NS
      locator = Pyro.naming.NameServerLocator()
      #print 'searching for Name Server...'
      ns = locator.getNS()
      daemon.useNameServer(ns)

      # connect a new object implementation (first unregister previous one)
      try:
            # 'test' is the name by which our object will be known to the outside world
            ns.unregister('muse')
      except NamingError:
            pass

      # connect new object implementation
      daemon.connect(museclass(),'muse')

      # enter the server loop.
      print 'Muse remote object published'
      daemon.requestLoop()

if __name__=="__main__":
        main()

main()


