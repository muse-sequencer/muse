"""
//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

from __future__ import print_function
import Pyro4.core
import Pyro4.naming
from Pyro4.errors import PyroError,NamingError
import socket
import select
import time
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
# Pyro >= 4.46, requires exposing now

class MusE:
      def __init__(self, daemon):
            self.daemon = daemon

      # This method is to support a technique of shutting down the server
      #  by calling this 'shutdown' from another thread or process.
      # From another process obviously tested OK. But it would be more desirable
      #  from another thread. But the code to do so from another thread in the SAME
      #  process can be complex (using sub-interpreters).
      # So our simpler loopCondition technique is the one used at this time of writing.
      # Still, we keep this around for possible usage since it does work.
      #
      # oneway in case call returns much later than daemon.shutdown
      @Pyro4.oneway
      @Pyro4.expose
      def shutdown(self):
            print('Shutting down Pyro daemon...')
            self.daemon.shutdown()

      #@staticmethod
      #@classmethod
      @Pyro4.expose
      def getCPos(self): # Get current position
            return muse.getCPos()

      @Pyro4.expose
      def startPlay(self): # Start playback
            return muse.startPlay()

      @Pyro4.expose
      def stopPlay(self): # Stop playback
            return muse.stopPlay()

      @Pyro4.expose
      def rewindStart(self): # Rewind current position to start
            return muse.rewindStart()

      @Pyro4.expose
      def getLPos(self): # Get position of left locator
            return muse.getLPos()

      @Pyro4.expose
      def getRPos(self): # Get position of right locator
            return muse.getRPos()

      @Pyro4.expose
      def getTempo(self, tick): #Get tempo at particular tick
            return muse.getTempo(tick)

      @Pyro4.expose
      def getTrackNames(self): # get track names
            return muse.getTrackNames()

      @Pyro4.expose
      def getParts(self, trackname): # get parts in a particular track
            return muse.getParts(trackname)

      @Pyro4.expose
      def createPart(self, trackname, starttick, lenticks, part): # create part in track
            return muse.createPart(trackname, starttick, lenticks, part)

      @Pyro4.expose
      def modifyPart(self, part): # modify a part (the part to be modified is specified by its id
            return muse.modifyPart((part))

      @Pyro4.expose
      def deletePart(self, part): # delete a part
            return muse.deletePart((part))

      @Pyro4.expose
      def getSelectedTrack(self): # get first selected track in arranger window
            return muse.getSelectedTrack()

      @Pyro4.expose
      def importPart(self, trackname, filename, tick): # import part file to a track at a given position
            return muse.importPart(trackname, filename, tick)

      @Pyro4.expose
      def setCPos(self, tick): # set current position
            return muse.setPos(0, tick)

      @Pyro4.expose
      def setLPos(self, tick): # set left locator
            return muse.setPos(1, tick)

      @Pyro4.expose
      def setRPos(self, tick): # set right locator
            return muse.setPos(2, tick)
      
      @Pyro4.expose
      def setSongLen(self, ticks): # set song length
            return muse.setSongLen(ticks)

      @Pyro4.expose
      def getSongLen(self): # get song length
            return muse.getSongLen()

      @Pyro4.expose
      def getDivision(self): # get division (ticks per 1/4, or per beat?)
            return muse.getDivision()

      @Pyro4.expose
      def setMidiTrackParameter(self, trackname, paramname, value): # set midi track parameter (velocity, compression, len, transpose)
            return muse.setMidiTrackParameter(trackname, paramname, value);

      @Pyro4.expose
      def getLoop(self): # get loop flag
            return muse.getLoop()

      @Pyro4.expose
      def setLoop(self, loopFlag): # set loop flag
            return muse.setLoop(loopFlag)
      
      @Pyro4.expose
      def getMute(self, trackname): # get track mute parameter
            return muse.getMute(trackname)

      @Pyro4.expose
      def setMute(self, trackname, enabled): # set track mute parameter
            return muse.setMute(trackname, enabled)

      @Pyro4.expose
      def setVolume(self, trackname, volume): # set mixer volume
            return muse.setVolume(trackname, volume)

      @Pyro4.expose
      def getMidiControllerValue(self, trackname, ctrlno): # get a particular midi controller value for a track
            return muse.getMidiControllerValue(trackname, ctrlno)

      @Pyro4.expose
      def setMidiControllerValue(self, trackname, ctrlno, value): # set a particular midi controller value for a track
            return muse.setMidiControllerValue(trackname, ctrlno, value)

      @Pyro4.expose
      def setAudioTrackVolume(self, trackname, dvol): # set volume for audio track 
            return muse.setAudioTrackVolume(trackname, dvol)

      @Pyro4.expose
      def getAudioTrackVolume(self, trackname): # get volume for audio track
            return muse.getAudioTrackVolume(trackname)

      @Pyro4.expose
      def getTrackEffects(self, trackname): # get effect names for an audio track
            return muse.getTrackEffects(trackname)

      @Pyro4.expose
      def toggleTrackEffect(self, trackname, effectno, onoff): # toggle specific effect on/off
            return muse.toggleTrackEffect(trackname, effectno, onoff)

      def findNewTrack(self, oldtracknames): #internal function
            tracknames = muse.getTrackNames()
            for trackname in tracknames:
                  if trackname in oldtracknames:
                        continue

                  return trackname

      @Pyro4.expose
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


      @Pyro4.expose
      def addMidiTrack(self, trackname): # add midi track
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addMidiTrack()
            self.nameNewTrack(trackname, oldtracknames)
            

      @Pyro4.expose
      def addWaveTrack(self, trackname): # add wave track
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addWaveTrack()
            self.nameNewTrack(trackname, oldtracknames)

      @Pyro4.expose
      def addInput(self, trackname): # add audio input
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addInput()
            self.nameNewTrack(trackname, oldtracknames)

      @Pyro4.expose
      def addOutput(self, trackname): # add audio output
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addOutput()
            self.nameNewTrack(trackname, oldtracknames)

      @Pyro4.expose
      def addGroup(self, trackname): # add audio group
            oldtracknames = muse.getTrackNames()
            if trackname in oldtracknames:
                  return None

            muse.addGroup()
            self.nameNewTrack(trackname, oldtracknames)

      @Pyro4.expose
      def deleteTrack(self, trackname): # delete a track
            tracknames = muse.getTrackNames()
            if trackname not in tracknames:
                  return False

            muse.deleteTrack(trackname)

#      def getOutputRoute(self, trackname):
#            return muse.getOutputRoute(trackname)

class NameServiceThread(threading.Thread):
      def __init__(self, host, port):
            threading.Thread.__init__(self)
            self.host = host
            self.port = port

      def run(self):
            hostname = socket.gethostname()
            print ("Starting Pyro nameserver")
            try:
                if self.port is None:
                    Pyro4.naming.startNSloop(host=self.host)
                else:
                    Pyro4.naming.startNSloop(host=self.host, port=self.port)
            except:
                print ("Unable to start Pyro nameserver at host=" + str(self.host) + " port=" + str(self.port))

            print ("Pyro nameserver finished")

#
# main server program
#
def main():
      print ("Inside museplauncher.py...")

      # TESTED: When running example scripts, too low a value caused problems with
      #  'not enough data' errors unless set to higher, around at least 3. But if set too high
      #  it takes longer for it to check loopCondition for our shutdown flag (tested 50 seconds,
      #  average shutdown wait was long). Be sure this value is LESS THAN the timeout value of
      #  the parent QThread's wait(value) so that the thread does not timeout before the daemon does.
      # This is done automatically in our c++ code by setting the QThread wait time to a higher
      #  value than our global MusEGlobal::pythonBridgePyroCommTimeout.
      #
      # We need to set either a socket communication timeout,
      #   or use the select based server. Otherwise the daemon requestLoop
      #   will block indefinitely and is never able to evaluate the loopCondition.
      # This is a floating point value.
      Pyro4.config.COMMTIMEOUT = muse.getConfigPyroCommTimeout()

      config_pyro_ns_hostname = muse.getConfigPyroNSHostname()
      config_pyro_ns_port = muse.getConfigPyroNSPort()
      config_pyro_daemon_hostname = muse.getConfigPyroDaemonHostname()
      config_pyro_daemon_port = muse.getConfigPyroDaemonPort()

      if config_pyro_ns_hostname:
          ns_hostname = config_pyro_ns_hostname
      else:
          #ns_hostname = socket.gethostname()
          ns_hostname = None

      if config_pyro_ns_port:
          ns_port = int(config_pyro_ns_port)
      else:
          ns_port = None

      if config_pyro_daemon_hostname:
          daemon_hostname = config_pyro_daemon_hostname
      else:
          #daemon_hostname = socket.gethostname()
          daemon_hostname = None

      if config_pyro_daemon_port:
          daemon_port = int(config_pyro_daemon_port)
      else:
          daemon_port = None

      print("Nameserver hostname:" + str(ns_hostname) + " Port:" + str(ns_port) + " Attempting to locate a Pyro nameserver...")

      try:
          ns = Pyro4.locateNS(host=ns_hostname, port=ns_port)
      except Pyro4.errors.NamingError:
          ns_running = False
      else:
          ns_running = True

      if ns_running:
          print("...Found Pyro nameserver")
      else:
          print("...Pyro nameserver not found. Starting a new one...")
          # Note: See the Pyro4 eventloop example for a cool custom loop that works,
          #  but it requires the broadcast server to be running. This does not.
          nsthread = NameServiceThread(host=ns_hostname, port=ns_port)
          nsthread.start()
          for i in range(0,5):
              try:
                  ns = Pyro4.locateNS(host=ns_hostname, port=ns_port)
              except Pyro4.errors.NamingError:
                  if i is 4:
                      print("...Pyro nameserver did not respond")
                      return 
                  time.sleep(1)
              else:
                break

      print("Daemon hostname:" + str(daemon_hostname) + " Port:" + str(daemon_port))
      try:
          if daemon_port is None:
              pyrodaemon = Pyro4.core.Daemon(host=daemon_hostname)
          else:
              pyrodaemon = Pyro4.core.Daemon(host=daemon_hostname, port=daemon_port)
      except:
          print("Could not start Pyro daemon")
          return
        
      print("Pyro daemon location string=%s" % pyrodaemon.locationStr)
      print("Pyro daemon sockets=%s" % pyrodaemon.sockets)

      # connect a new object implementation (first unregister previous one)
      try:
          ns.remove('muse')
      except Pyro4.errors.NamingError:
          pass

      muse_inst = MusE(pyrodaemon)

      # register a server object with the daemon
      serveruri = pyrodaemon.register(muse_inst, objectId="muse")
      print("Object registered with Pyro daemon. uri=%s" % serveruri)

      # register it with the embedded nameserver directly
      ns.register("muse", serveruri)
      print("Object registered with the Pyro nameserver")

      # Tested OK.
      pyrodaemon.requestLoop(lambda : muse.serverRunFlag())

      print("Pyro daemon loop finished")

# We only want execution if running the script. The value is set to __main__.
# Otherwise if being imported by another module __name__ contains the name of the script.
if __name__=="__main__":
        main()

# FIXME What was this for? It caused double execution in cases
#        where the server didn't start (which makes sense)
#main()


