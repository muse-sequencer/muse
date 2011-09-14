//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: conf.cpp,v 1.33.2.18 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <sndfile.h>
#include <errno.h>
#include <stdio.h>

#include "app.h"
#include "transport.h"
#include "icons.h"
#include "globals.h"
#include "functions.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "scoreedit.h"
#include "master/masteredit.h"
#include "listedit.h"
#include "cliplist/cliplist.h"
#include "arrangerview.h"
#include "marker/markerview.h"
#include "master/lmaster.h"
///#include "transport.h"
#include "bigtime.h"
#include "arranger.h"
#include "conf.h"
#include "gconfig.h"
#include "pitchedit.h"
#include "midiport.h"
#include "mididev.h"
#include "driver/audiodev.h"
#include "driver/jackmidi.h"
#include "xml.h"
#include "waveedit.h"
#include "midi.h"
#include "midisyncimpl.h"
#include "midifilterimpl.h"
#include "midictrl.h"
#include "ctrlcombo.h"
#include "genset.h"
#include "midiitransform.h"
#include "synth.h"
#include "audio.h"
#include "sync.h"
#include "wave.h"
#include "midiseq.h"
#include "amixer.h"
#include "track.h"

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);

extern void writeMidiInputTransforms(int level, Xml& xml);
extern void readMidiInputTransform(Xml&);

//---------------------------------------------------------
//   readGeometry
//---------------------------------------------------------

QRect readGeometry(Xml& xml, const QString& name)
      {
      QRect r(0, 0, 50, 50);
      int val;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        xml.parse1();
                        break;
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "x")
                              r.setX(val);
                        else if (tag == "y")
                              r.setY(val);
                        else if (tag == "w")
                              r.setWidth(val);
                        else if (tag == "h")
                              r.setHeight(val);
                        break;
                  case Xml::TagEnd:
                        if (tag == name)
                              return r;
                  default:
                        break;
                  }
            }
      return r;
      }


//---------------------------------------------------------
//   readColor
//---------------------------------------------------------

QColor readColor(Xml& xml)
       {
       int val, r=0, g=0, b=0;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token != Xml::Attribut)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "r")
                              r = val;
                        else if (tag == "g")
                              g = val;
                        else if (tag == "b")
                              b = val;
                        break;
                  default:
                        break;
                  }
            }

      return QColor(r, g, b);
      }

//---------------------------------------------------------
//   readController
//---------------------------------------------------------

static void readController(Xml& xml, int midiPort, int channel)
      {
      int id = 0;
      int val = CTRL_VAL_UNKNOWN;

      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "val")
                              val = xml.parseInt();
                        else
                              xml.unknown("controller");
                        break;
                  case Xml::Attribut:
                        if (tag == "id")
                              id = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "controller") {
                              MidiPort* port = &midiPorts[midiPort];
                              //port->addManagedController(channel, id);
                              val = port->limitValToInstrCtlRange(id, val);
                              // The value here will actually be sent to the device LATER, in MidiPort::setMidiDevice()
                              port->setHwCtrlState(channel, id, val);
                              return;
                              }
                  default:
                        return;
                  }
            }
      }

//---------------------------------------------------------
//   readPortChannel
//---------------------------------------------------------

static void readPortChannel(Xml& xml, int midiPort)
      {
      int idx = 0;  //torbenh
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "controller") {
                              readController(xml, midiPort, idx);
                              }
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        if (tag == "idx")
                              idx = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "channel")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfigMidiPort
//---------------------------------------------------------

static void readConfigMidiPort(Xml& xml)
      {
      int idx = 0;
      QString device;
      
      //QString instrument;
      // Changed by Tim. 
      //QString instrument("generic midi"); 
      // Let's be bold. New users have been confused by generic midi not enabling any patches and controllers.
      // I had said this may cause HW problems by sending out GM sysEx when really the HW might not be GM.
      // But this really needs to be done, one way or another. 
      // FIXME: TODO: Make this user-configurable!
      QString instrument("GM"); 
      
      int openFlags = 1;
      bool thruFlag = false;
      //int dic = 0;
      //int doc = 0;
      int dic = -1;   // p4.0.17
      int doc = -1;
      
      MidiSyncInfo tmpSi;
      int type = MidiDevice::ALSA_MIDI;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "name")
                              device = xml.parse1();
                        else if (tag == "type")
                              type = xml.parseInt();
                        else if (tag == "record") {         // old
                              bool f = xml.parseInt();
                              if (f)
                                    openFlags |= 2;
                              }
                        else if (tag == "openFlags")
                              openFlags = xml.parseInt();
                        else if (tag == "defaultInChans")
                              dic = xml.parseInt(); 
                        else if (tag == "defaultOutChans")
                              doc = xml.parseInt(); 
                        else if (tag == "midiSyncInfo")
                              tmpSi.read(xml);
                        else if (tag == "instrument") {
                              instrument = xml.parse1();
                              // Moved by Tim.
                              //midiPorts[idx].setInstrument(
                              //   registerMidiInstrument(instrument)
                              //   );
                              }
                        else if (tag == "midithru")
                              thruFlag = xml.parseInt(); // obsolete
                        else if (tag == "channel") {
                              readPortChannel(xml, idx);
                              }
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        if (tag == "idx") {
                              idx = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "midiport") {
                              //if (idx > MIDI_PORTS) {
                              if (idx < 0 || idx >= MIDI_PORTS) {
                                    fprintf(stderr, "bad midi port %d (>%d)\n",
                                       idx, MIDI_PORTS);
                                    idx = 0;
                                    }
                              
                              MidiDevice* dev = midiDevices.find(device);
                              
                              //if(MusEGlobal::debugMsg && !dev)
                              //  fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.toLatin1().constData());
                                
                              if(!dev && type == MidiDevice::JACK_MIDI)
                              {
                                if(MusEGlobal::debugMsg)
                                  fprintf(stderr, "readConfigMidiPort: creating jack midi device %s\n", device.toLatin1().constData());
                                //dev = MidiJackDevice::createJackMidiDevice(device, openFlags);
                                dev = MidiJackDevice::createJackMidiDevice(device);  // p3.3.55
                              }
                              
                              if(MusEGlobal::debugMsg && !dev)
                                fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.toLatin1().constData());
                              
                              MidiPort* mp = &midiPorts[idx];
                              
                              mp->setInstrument(registerMidiInstrument(instrument));  // By Tim.
                              if(dic != -1)                      // p4.0.17 Leave them alone unless set by song.
                                mp->setDefaultInChannels(dic);
                              if(doc != -1)
                                // p4.0.17 Turn on if and when multiple output routes supported.
                                #if 0
                                mp->setDefaultOutChannels(doc);
                                #else
                                setPortExclusiveDefOutChan(idx, doc);
                                #endif
                                
                              mp->syncInfo().copyParams(tmpSi);
                              // p3.3.50 Indicate the port was found in the song file, even if no device is assigned to it.
                              mp->setFoundInSongFile(true);
                              
                              if (dev) {
                                    dev->setOpenFlags(openFlags);
                                    midiSeq->msgSetMidiDevice(mp, dev);
                                    }
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

/*
//---------------------------------------------------------
//   readConfigMidiSyncInfo
//---------------------------------------------------------

static void readConfigMidiSyncInfo(Xml& xml)
{
      QString device;
      int idOut       = 127;
      int idIn        = 127;
      bool sendMC     = false;
      bool sendMMC    = false;
      bool sendMTC    = false;
      bool recMC      = false;
      bool recMMC     = false;
      bool recMTC     = false;
      
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "device")
                              device = xml.parse1();
                        else if (tag == "idOut")
                              idOut = (xml.parseInt());
                        else if (tag == "idIn")
                              idIn = xml.parseInt();
                        else if (tag == "sendMC")
                              sendMC = xml.parseInt();
                        else if (tag == "sendMMC")
                              sendMMC = xml.parseInt();
                        else if (tag == "sendMTC")
                              sendMTC = xml.parseInt();
                        else if (tag == "recMC")
                              recMC = xml.parseInt();
                        else if (tag == "recMMC")
                              recMMC = xml.parseInt();
                        else if (tag == "recMTC")
                              recMTC = xml.parseInt();
                        else
                              xml.unknown("midiSyncInfo");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if(tag == "midiSyncInfo") 
                        {
                          MidiDevice* dev = midiDevices.find(device);
                          if(dev) 
                          {
                            MidiSyncInfo& si = dev->syncInfo();
                            si.setIdIn(idIn);
                            si.setIdOut(idOut);
                            
                            si.setMCIn(recMC);
                            si.setMMCIn(recMMC);
                            si.setMTCIn(recMTC);
                            
                            si.setMCOut(sendMC);
                            si.setMMCOut(sendMMC);
                            si.setMTCOut(sendMTC);
                          }
                          else
                            fprintf(stderr, "Read configuration: Sync device: %s not found\n", device.toLatin1().constData());
                            
                          return;
                        }
                  default:
                        break;
                  }
            }
}
*/

//---------------------------------------------------------
//   loadConfigMetronom
//---------------------------------------------------------

static void loadConfigMetronom(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "premeasures")
                              MusEGlobal::preMeasures = xml.parseInt();
                        else if (tag == "measurepitch")
                              MusEGlobal::measureClickNote = xml.parseInt();
                        else if (tag == "measurevelo")
                              MusEGlobal::measureClickVelo = xml.parseInt();
                        else if (tag == "beatpitch")
                              MusEGlobal::beatClickNote = xml.parseInt();
                        else if (tag == "beatvelo")
                              MusEGlobal::beatClickVelo = xml.parseInt();
                        else if (tag == "channel")
                              MusEGlobal::clickChan = xml.parseInt();
                        else if (tag == "port")
                              MusEGlobal::clickPort = xml.parseInt();
                        else if (tag == "precountEnable")
                              MusEGlobal::precountEnableFlag = xml.parseInt();
                        else if (tag == "fromMastertrack")
                              MusEGlobal::precountFromMastertrackFlag = xml.parseInt();
                        else if (tag == "signatureZ")
                              MusEGlobal::precountSigZ = xml.parseInt();
                        else if (tag == "signatureN")
                              MusEGlobal::precountSigN = xml.parseInt();
                        else if (tag == "prerecord")
                              MusEGlobal::precountPrerecord = xml.parseInt();
                        else if (tag == "preroll")
                              MusEGlobal::precountPreroll = xml.parseInt();
                        else if (tag == "midiClickEnable")
                              MusEGlobal::midiClickFlag = xml.parseInt();
                        else if (tag == "audioClickEnable")
                              MusEGlobal::audioClickFlag = xml.parseInt();
                        else if (tag == "audioClickVolume")
                              MusEGlobal::audioClickVolume = xml.parseFloat();
                        else
                              xml.unknown("Metronome");
                        break;
                  case Xml::TagEnd:
                        if (tag == "metronom")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readSeqConfiguration
//---------------------------------------------------------

static void readSeqConfiguration(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "metronom")
                              loadConfigMetronom(xml);
                        else if (tag == "midiport")
                              readConfigMidiPort(xml);
                        else if (tag == "rcStop")
                              MusEGlobal::rcStopNote = xml.parseInt();
                        else if (tag == "rcEnable")
                              MusEGlobal::rcEnable = xml.parseInt();
                        else if (tag == "rcRecord")
                              MusEGlobal::rcRecordNote = xml.parseInt();
                        else if (tag == "rcGotoLeft")
                              MusEGlobal::rcGotoLeftMarkNote = xml.parseInt();
                        else if (tag == "rcPlay")
                              MusEGlobal::rcPlayNote = xml.parseInt();
                        else if (tag == "rcSteprec")
                              MusEGlobal::rcSteprecNote = xml.parseInt();
                        else
                              xml.unknown("Seq");
                        break;
                  case Xml::TagEnd:
                        if (tag == "sequencer") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void readConfiguration(Xml& xml, bool readOnlySequencer)
      {
      int mixers = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        /* the reading of configuration is split in two; read
                           "sequencer" and read ALL. The reason is that it is
                           possible to load a song without configuration. In
                           this case the <configuration> chapter in the song
                           file should be skipped. However the sub part
                           <sequencer> contains elements that are necessary
                           to preserve composition consistency. Mainly
                           midiport configuration and VOLUME.
                        */
                        if (tag == "sequencer") {
                              readSeqConfiguration(xml);
                              break;
                              }
                        else if (readOnlySequencer) {
                              xml.skip(tag);
                              break;
                              }
                        
                        if (tag == "theme")
                              MusEConfig::config.style = xml.parse1();
                        else if (tag == "styleSheetFile")
                              MusEConfig::config.styleSheetFile = xml.parse1();
                        else if (tag == "useOldStyleStopShortCut")
                              MusEConfig::config.useOldStyleStopShortCut = xml.parseInt();
                        else if (tag == "moveArmedCheckBox")
                              MusEConfig::config.moveArmedCheckBox = xml.parseInt();
                        else if (tag == "externalWavEditor")
                              MusEConfig::config.externalWavEditor = xml.parse1();
                        else if (tag == "font0")
                              MusEConfig::config.fonts[0].fromString(xml.parse1());
                        else if (tag == "font1")
                              MusEConfig::config.fonts[1].fromString(xml.parse1());
                        else if (tag == "font2")
                              MusEConfig::config.fonts[2].fromString(xml.parse1());
                        else if (tag == "font3")
                              MusEConfig::config.fonts[3].fromString(xml.parse1());
                        else if (tag == "font4")
                              MusEConfig::config.fonts[4].fromString(xml.parse1());
                        else if (tag == "font5")
                              MusEConfig::config.fonts[5].fromString(xml.parse1());
                        else if (tag == "font6")
                              MusEConfig::config.fonts[6].fromString(xml.parse1());
                        else if (tag == "globalAlphaBlend")
                              MusEConfig::config.globalAlphaBlend = xml.parseInt();
                        else if (tag == "palette0")
                              MusEConfig::config.palette[0] = readColor(xml);
                        else if (tag == "palette1")
                              MusEConfig::config.palette[1] = readColor(xml);
                        else if (tag == "palette2")
                              MusEConfig::config.palette[2] = readColor(xml);
                        else if (tag == "palette3")
                              MusEConfig::config.palette[3] = readColor(xml);
                        else if (tag == "palette4")
                              MusEConfig::config.palette[4] = readColor(xml);
                        else if (tag == "palette5")
                              MusEConfig::config.palette[5] = readColor(xml);
                        else if (tag == "palette6")
                              MusEConfig::config.palette[6] = readColor(xml);
                        else if (tag == "palette7")
                              MusEConfig::config.palette[7] = readColor(xml);
                        else if (tag == "palette8")
                              MusEConfig::config.palette[8] = readColor(xml);
                        else if (tag == "palette9")
                              MusEConfig::config.palette[9] = readColor(xml);
                        else if (tag == "palette10")
                              MusEConfig::config.palette[10] = readColor(xml);
                        else if (tag == "palette11")
                              MusEConfig::config.palette[11] = readColor(xml);
                        else if (tag == "palette12")
                              MusEConfig::config.palette[12] = readColor(xml);
                        else if (tag == "palette13")
                              MusEConfig::config.palette[13] = readColor(xml);
                        else if (tag == "palette14")
                              MusEConfig::config.palette[14] = readColor(xml);
                        else if (tag == "palette15")
                              MusEConfig::config.palette[15] = readColor(xml);
                        else if (tag == "palette16")
                              MusEConfig::config.palette[16] = readColor(xml);
                        else if (tag == "partColor0")
                              MusEConfig::config.partColors[0] = readColor(xml);
                        else if (tag == "partColor1")
                              MusEConfig::config.partColors[1] = readColor(xml);
                        else if (tag == "partColor2")
                              MusEConfig::config.partColors[2] = readColor(xml);
                        else if (tag == "partColor3")
                              MusEConfig::config.partColors[3] = readColor(xml);
                        else if (tag == "partColor4")
                              MusEConfig::config.partColors[4] = readColor(xml);
                        else if (tag == "partColor5")
                              MusEConfig::config.partColors[5] = readColor(xml);
                        else if (tag == "partColor6")
                              MusEConfig::config.partColors[6] = readColor(xml);
                        else if (tag == "partColor7")
                              MusEConfig::config.partColors[7] = readColor(xml);
                        else if (tag == "partColor8")
                              MusEConfig::config.partColors[8] = readColor(xml);
                        else if (tag == "partColor9")
                              MusEConfig::config.partColors[9] = readColor(xml);
                        else if (tag == "partColor10")
                              MusEConfig::config.partColors[10] = readColor(xml);
                        else if (tag == "partColor11")
                              MusEConfig::config.partColors[11] = readColor(xml);
                        else if (tag == "partColor12")
                              MusEConfig::config.partColors[12] = readColor(xml);
                        else if (tag == "partColor13")
                              MusEConfig::config.partColors[13] = readColor(xml);
                        else if (tag == "partColor14")
                              MusEConfig::config.partColors[14] = readColor(xml);
                        else if (tag == "partColor15")
                              MusEConfig::config.partColors[15] = readColor(xml);
                        else if (tag == "partColor16")
                              MusEConfig::config.partColors[16] = readColor(xml);
                        else if (tag == "partColor17")
                              MusEConfig::config.partColors[17] = readColor(xml);
                        
                        else if (tag == "partColorName0")
                              MusEConfig::config.partColorNames[0] = xml.parse1();
                        else if (tag == "partColorName1")
                              MusEConfig::config.partColorNames[1] = xml.parse1();
                        else if (tag == "partColorName2")
                              MusEConfig::config.partColorNames[2] = xml.parse1();
                        else if (tag == "partColorName3")
                              MusEConfig::config.partColorNames[3] = xml.parse1();
                        else if (tag == "partColorName4")
                              MusEConfig::config.partColorNames[4] = xml.parse1();
                        else if (tag == "partColorName5")
                              MusEConfig::config.partColorNames[5] = xml.parse1();
                        else if (tag == "partColorName6")
                              MusEConfig::config.partColorNames[6] = xml.parse1();
                        else if (tag == "partColorName7")
                              MusEConfig::config.partColorNames[7] = xml.parse1();
                        else if (tag == "partColorName8")
                              MusEConfig::config.partColorNames[8] = xml.parse1();
                        else if (tag == "partColorName9")
                              MusEConfig::config.partColorNames[9] = xml.parse1();
                        else if (tag == "partColorName10")
                              MusEConfig::config.partColorNames[10] = xml.parse1();
                        else if (tag == "partColorName11")
                              MusEConfig::config.partColorNames[11] = xml.parse1();
                        else if (tag == "partColorName12")
                              MusEConfig::config.partColorNames[12] = xml.parse1();
                        else if (tag == "partColorName13")
                              MusEConfig::config.partColorNames[13] = xml.parse1();
                        else if (tag == "partColorName14")
                              MusEConfig::config.partColorNames[14] = xml.parse1();
                        else if (tag == "partColorName15")
                              MusEConfig::config.partColorNames[15] = xml.parse1();
                        else if (tag == "partColorName16")
                              MusEConfig::config.partColorNames[16] = xml.parse1();
                        else if (tag == "partColorName17")
                              MusEConfig::config.partColorNames[17] = xml.parse1();
                        
                        else if (tag == "partCanvasBg")
                              MusEConfig::config.partCanvasBg = readColor(xml);
                        else if (tag == "trackBg")
                              MusEConfig::config.trackBg = readColor(xml);
                        else if (tag == "selectTrackBg")
                              MusEConfig::config.selectTrackBg = readColor(xml);
                        else if (tag == "selectTrackFg")
                              MusEConfig::config.selectTrackFg = readColor(xml);
                        
                        else if (tag == "mixerBg")
                              MusEConfig::config.mixerBg = readColor(xml);
                        else if (tag == "midiTrackLabelBg")
                              MusEConfig::config.midiTrackLabelBg = readColor(xml);
                        else if (tag == "drumTrackLabelBg")
                              MusEConfig::config.drumTrackLabelBg = readColor(xml);
                        else if (tag == "waveTrackLabelBg")
                              MusEConfig::config.waveTrackLabelBg = readColor(xml);
                        else if (tag == "outputTrackLabelBg")
                              MusEConfig::config.outputTrackLabelBg = readColor(xml);
                        else if (tag == "inputTrackLabelBg")
                              MusEConfig::config.inputTrackLabelBg = readColor(xml);
                        else if (tag == "groupTrackLabelBg")
                              MusEConfig::config.groupTrackLabelBg = readColor(xml);
                        else if (tag == "auxTrackLabelBg")
                              MusEConfig::config.auxTrackLabelBg = readColor(xml);
                        else if (tag == "synthTrackLabelBg")
                              MusEConfig::config.synthTrackLabelBg = readColor(xml);
                        
                        else if (tag == "midiTrackBg")
                              MusEConfig::config.midiTrackBg = readColor(xml);
                        else if (tag == "ctrlGraphFg")
                              MusEConfig::config.ctrlGraphFg = readColor(xml);
                        else if (tag == "drumTrackBg")
                              MusEConfig::config.drumTrackBg = readColor(xml);
                        else if (tag == "waveTrackBg")
                              MusEConfig::config.waveTrackBg = readColor(xml);
                        else if (tag == "outputTrackBg")
                              MusEConfig::config.outputTrackBg = readColor(xml);
                        else if (tag == "inputTrackBg")
                              MusEConfig::config.inputTrackBg = readColor(xml);
                        else if (tag == "groupTrackBg")
                              MusEConfig::config.groupTrackBg = readColor(xml);
                        else if (tag == "auxTrackBg")
                              MusEConfig::config.auxTrackBg = readColor(xml);
                        else if (tag == "synthTrackBg")
                              MusEConfig::config.synthTrackBg = readColor(xml);
                        
                        else if (tag == "extendedMidi")
                              MusEConfig::config.extendedMidi = xml.parseInt();
                        else if (tag == "midiExportDivision")
                              MusEConfig::config.midiDivision = xml.parseInt();
                        else if (tag == "copyright")
                              MusEConfig::config.copyright = xml.parse1();
                        else if (tag == "smfFormat")
                              MusEConfig::config.smfFormat = xml.parseInt();
                        else if (tag == "exp2ByteTimeSigs")
                              MusEConfig::config.exp2ByteTimeSigs = xml.parseInt();
                        else if (tag == "expOptimNoteOffs")
                              MusEConfig::config.expOptimNoteOffs = xml.parseInt();
                        else if (tag == "importMidiSplitParts")
                              MusEConfig::config.importMidiSplitParts = xml.parseInt();
                        else if (tag == "midiInputDevice")
                              MusEGlobal::midiInputPorts = xml.parseInt();
                        else if (tag == "midiInputChannel")
                              MusEGlobal::midiInputChannel = xml.parseInt();
                        else if (tag == "midiRecordType")
                              MusEGlobal::midiRecordType = xml.parseInt();
                        else if (tag == "midiThruType")
                              MusEGlobal::midiThruType = xml.parseInt();
                        else if (tag == "midiFilterCtrl1")
                              MusEGlobal::midiFilterCtrl1 = xml.parseInt();
                        else if (tag == "midiFilterCtrl2")
                              MusEGlobal::midiFilterCtrl2 = xml.parseInt();
                        else if (tag == "midiFilterCtrl3")
                              MusEGlobal::midiFilterCtrl3 = xml.parseInt();
                        else if (tag == "midiFilterCtrl4")
                              MusEGlobal::midiFilterCtrl4 = xml.parseInt();
                        else if (tag == "waveTracksVisible")
                                 WaveTrack::setVisible((bool)xml.parseInt());
                        else if (tag == "auxTracksVisible")
                                 AudioAux::setVisible((bool)xml.parseInt());
                        else if (tag == "groupTracksVisible")
                                 AudioGroup::setVisible((bool)xml.parseInt());
                        else if (tag == "midiTracksVisible")
                                 MidiTrack::setVisible((bool)xml.parseInt());
                        else if (tag == "inputTracksVisible")
                                 AudioInput::setVisible((bool)xml.parseInt());
                        else if (tag == "outputTracksVisible")
                                 AudioOutput::setVisible((bool)xml.parseInt());
                        else if (tag == "synthTracksVisible")
                                 SynthI::setVisible((bool)xml.parseInt());
                        else if (tag == "bigtimeVisible")
                              MusEConfig::config.bigTimeVisible = xml.parseInt();
                        else if (tag == "transportVisible")
                              MusEConfig::config.transportVisible = xml.parseInt();
                        else if (tag == "markerVisible")
                              MusEConfig::config.markerVisible = xml.parseInt();
                        
                        else if (tag == "mixerVisible")
                              // MusEConfig::config.mixerVisible = xml.parseInt();  // Obsolete
                              xml.skip(tag);
                        else if (tag == "mixer1Visible")
                              MusEConfig::config.mixer1Visible = xml.parseInt();
                        else if (tag == "mixer2Visible")
                              MusEConfig::config.mixer2Visible = xml.parseInt();
                        
                        else if (tag == "showSplashScreen")
                              MusEConfig::config.showSplashScreen = xml.parseInt();
                        else if (tag == "canvasShowPartType")
                              MusEConfig::config.canvasShowPartType = xml.parseInt();
                        else if (tag == "canvasShowPartEvent")
                              MusEConfig::config.canvasShowPartEvent = xml.parseInt();
                        else if (tag == "canvasShowGrid")
                              MusEConfig::config.canvasShowGrid = xml.parseInt();
                        else if (tag == "canvasBgPixmap")
                              MusEConfig::config.canvasBgPixmap = xml.parse1();
                        else if (tag == "canvasCustomBgList")
                              MusEConfig::config.canvasCustomBgList = xml.parse1().split(";", QString::SkipEmptyParts);
                        else if (tag == "geometryMain")
                              MusEConfig::config.geometryMain = readGeometry(xml, tag);
                        else if (tag == "geometryTransport")
                              MusEConfig::config.geometryTransport = readGeometry(xml, tag);
                        else if (tag == "geometryBigTime")
                              MusEConfig::config.geometryBigTime = readGeometry(xml, tag);
                        else if (tag == "geometryPianoroll")
                              MusEConfig::config.geometryPianoroll = readGeometry(xml, tag);
                        else if (tag == "geometryDrumedit")
                              MusEConfig::config.geometryDrumedit = readGeometry(xml, tag);
                        
                        else if (tag == "geometryMixer")
                              // MusEConfig::config.geometryMixer = readGeometry(xml, tag); // Obsolete
                              xml.skip(tag);
                        //else if (tag == "mixer1")
                        //      MusEConfig::config.mixer1.read(xml);
                        //else if (tag == "mixer2")
                        //      MusEConfig::config.mixer2.read(xml);
                        else if (tag == "Mixer")
                        {
                              if(mixers == 0)
                                MusEConfig::config.mixer1.read(xml);
                              else  
                                MusEConfig::config.mixer2.read(xml);
                              ++mixers;
                        }
                        
                        else if (tag == "bigtimeForegroundcolor")
                              MusEConfig::config.bigTimeForegroundColor = readColor(xml);
                        else if (tag == "bigtimeBackgroundcolor")
                              MusEConfig::config.bigTimeBackgroundColor = readColor(xml);
                        else if (tag == "transportHandleColor")
                              MusEConfig::config.transportHandleColor = readColor(xml);
                        else if (tag == "waveEditBackgroundColor")
                              MusEConfig::config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "txDeviceId")
                                //txDeviceId = xml.parseInt();
                                xml.parseInt();
                        else if (tag == "rxDeviceId")
                                //rxDeviceId = xml.parseInt();
                                xml.parseInt();
                        else if (tag == "txSyncPort")
                                //txSyncPort= xml.parseInt();
                                xml.parseInt();
                        else if (tag == "rxSyncPort")
                                //rxSyncPort= xml.parseInt();
                                xml.parseInt();
                        else if (tag == "mtctype")
                              mtcType= xml.parseInt();
                        else if (tag == "sendClockDelay")
                              syncSendFirstClockDelay = xml.parseUInt();
                        else if (tag == "extSync")
                              extSyncFlag.setValue(xml.parseInt());
                        else if (tag == "useJackTransport")
                              {
                              useJackTransport.setValue(xml.parseInt());
                              }
                        else if (tag == "jackTransportMaster")
                              {
                                jackTransportMaster = xml.parseInt();
                                if(audioDevice)
                                  audioDevice->setMaster(jackTransportMaster);      
                              }  
                        else if (tag == "syncgentype") {
                              // for compatibility
                              //int syncGenType= xml.parseInt();
                              //genMTCSync = syncGenType == 1;
                              //genMCSync = syncGenType == 2;
                              xml.parseInt();
                              }
                        else if (tag == "genMTCSync")
                              //genMTCSync = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "genMCSync")
                              //genMCSync = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "genMMC")
                              //genMMC = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "acceptMTC")
                              //acceptMTC = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "acceptMMC")
                              //acceptMMC = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "acceptMC")
                              //acceptMC = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "mtcoffset") {
                              QString qs(xml.parse1());
                              QByteArray ba = qs.toLatin1();
                              const char* str = ba.constData();
                              int h, m, s, f, sf;
                              sscanf(str, "%d:%d:%d:%d:%d", &h, &m, &s, &f, &sf);
                              mtcOffset = MTC(h, m, s, f, sf);
                              }
                        //else if (tag == "midiSyncInfo")
                        //      readConfigMidiSyncInfo(xml);
                        else if (tag == "drumedit")
                              DrumEdit::readConfiguration(xml);
                        else if (tag == "pianoroll")
                              PianoRoll::readConfiguration(xml);
                        else if (tag == "scoreedit")
                              ScoreEdit::read_configuration(xml);
                        else if (tag == "masteredit")
                              MasterEdit::readConfiguration(xml);
                        else if (tag == "waveedit")
                              WaveEdit::readConfiguration(xml);
                        else if (tag == "listedit")
                              ListEdit::readConfiguration(xml);
                        else if (tag == "cliplistedit")
                              ClipListEdit::readConfiguration(xml);
                        else if (tag == "lmaster")
                              LMaster::readConfiguration(xml);
                        else if (tag == "marker")
                              MarkerView::readConfiguration(xml);
                        else if (tag == "arrangerview")
                              ArrangerView::readConfiguration(xml);
                        else if (tag == "arranger") {
                              if (MusEGlobal::muse && MusEGlobal::muse->arranger())
                                    MusEGlobal::muse->arranger()->readStatus(xml);
                              else
                                    xml.skip(tag);
                              }
                        else if (tag == "dialogs")
                              read_function_dialog_config(xml);
                        else if (tag == "shortcuts")
                              readShortCuts(xml);
                        else if (tag == "division")
                              MusEConfig::config.division = xml.parseInt();
                        else if (tag == "guiDivision")
                              MusEConfig::config.guiDivision = xml.parseInt();
                        else if (tag == "samplerate")
                              xml.parseInt();
                        else if (tag == "segmentsize")
                              xml.parseInt();
                        else if (tag == "segmentcount")
                              xml.parseInt();
                        else if (tag == "rtcTicks")
                              MusEConfig::config.rtcTicks = xml.parseInt();
                        else if (tag == "minMeter")
                              MusEConfig::config.minMeter = xml.parseInt();
                        else if (tag == "minSlider")
                              MusEConfig::config.minSlider = xml.parseDouble();
                        else if (tag == "freewheelMode")
                              MusEConfig::config.freewheelMode = xml.parseInt();
                        else if (tag == "denormalProtection")
                              MusEConfig::config.useDenormalBias = xml.parseInt();
                        else if (tag == "didYouKnow")
                              MusEConfig::config.showDidYouKnow = xml.parseInt();
                        else if (tag == "outputLimiter")
                              MusEConfig::config.useOutputLimiter = xml.parseInt();
                        else if (tag == "vstInPlace")
                              MusEConfig::config.vstInPlace = xml.parseInt();
                        else if (tag == "dummyAudioSampleRate")
                              MusEConfig::config.dummyAudioSampleRate = xml.parseInt();
                        else if (tag == "dummyAudioBufSize")
                              MusEConfig::config.dummyAudioBufSize = xml.parseInt();
                        else if (tag == "minControlProcessPeriod")
                              MusEConfig::config.minControlProcessPeriod = xml.parseUInt();
                        else if (tag == "guiRefresh")
                              MusEConfig::config.guiRefresh = xml.parseInt();
                        else if (tag == "userInstrumentsDir")
                              MusEConfig::config.userInstrumentsDir = xml.parse1();
                        else if (tag == "midiTransform")
                              readMidiTransform(xml);
                        else if (tag == "midiInputTransform")
                              readMidiInputTransform(xml);
                        else if (tag == "startMode")
                              MusEConfig::config.startMode = xml.parseInt();
                        else if (tag == "startSong")
                              MusEConfig::config.startSong = xml.parse1();
                        else if (tag == "projectBaseFolder")
                              MusEConfig::config.projectBaseFolder = xml.parse1();
                        else if (tag == "projectStoreInFolder")
                              MusEConfig::config.projectStoreInFolder = xml.parseInt();
                        else if (tag == "useProjectSaveDialog")
                              MusEConfig::config.useProjectSaveDialog = xml.parseInt();
                        else if (tag == "popupsDefaultStayOpen")
                              MusEConfig::config.popupsDefaultStayOpen = xml.parseInt();

                        else
                              xml.unknown("configuration");
                        break;
                  case Xml::Text:
                        printf("text <%s>\n", xml.s1().toLatin1().constData());
                        break;
                  case Xml::Attribut:
                        if (readOnlySequencer)
                              break;
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "configuration") {
                              return;
                              }
                        break;
                  case Xml::Proc:
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

bool readConfiguration()
      {
      FILE* f = fopen(MusEGlobal::configName.toLatin1().constData(), "r");
      if (f == 0) {
            if (MusEGlobal::debugMsg || MusEGlobal::debugMode)
                  fprintf(stderr, "NO Config File <%s> found\n", MusEGlobal::configName.toLatin1().constData());

            if (MusEConfig::config.userInstrumentsDir.isEmpty()) 
                  MusEConfig::config.userInstrumentsDir = MusEGlobal::configPath + "/instruments";
            return true;
            }
      Xml xml(f);
      bool skipmode = true;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        fclose(f);
                        return true;
                  case Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              readConfiguration(xml,false);
                        else
                              xml.unknown("muse config");
                        break;
                  case Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
                        if (!skipmode && tag == "muse") {
                              fclose(f);
                              return false;
                              }
                  default:
                        break;
                  }
            }
      fclose(f);
      return true;
      }

//---------------------------------------------------------
//   writeSeqConfiguration
//---------------------------------------------------------

static void writeSeqConfiguration(int level, Xml& xml, bool writePortInfo)
      {
      xml.tag(level++, "sequencer");

      xml.tag(level++, "metronom");
      xml.intTag(level, "premeasures", MusEGlobal::preMeasures);
      xml.intTag(level, "measurepitch", MusEGlobal::measureClickNote);
      xml.intTag(level, "measurevelo", MusEGlobal::measureClickVelo);
      xml.intTag(level, "beatpitch", MusEGlobal::beatClickNote);
      xml.intTag(level, "beatvelo", MusEGlobal::beatClickVelo);
      xml.intTag(level, "channel", MusEGlobal::clickChan);
      xml.intTag(level, "port", MusEGlobal::clickPort);

      xml.intTag(level, "precountEnable", MusEGlobal::precountEnableFlag);
      xml.intTag(level, "fromMastertrack", MusEGlobal::precountFromMastertrackFlag);
      xml.intTag(level, "signatureZ", MusEGlobal::precountSigZ);
      xml.intTag(level, "signatureN", MusEGlobal::precountSigN);
      xml.intTag(level, "prerecord", MusEGlobal::precountPrerecord);
      xml.intTag(level, "preroll", MusEGlobal::precountPreroll);
      xml.intTag(level, "midiClickEnable", MusEGlobal::midiClickFlag);
      xml.intTag(level, "audioClickEnable", MusEGlobal::audioClickFlag);
      xml.floatTag(level, "audioClickVolume", MusEGlobal::audioClickVolume);
      xml.tag(level--, "/metronom");

      xml.intTag(level, "rcEnable",   MusEGlobal::rcEnable);
      xml.intTag(level, "rcStop",     MusEGlobal::rcStopNote);
      xml.intTag(level, "rcRecord",   MusEGlobal::rcRecordNote);
      xml.intTag(level, "rcGotoLeft", MusEGlobal::rcGotoLeftMarkNote);
      xml.intTag(level, "rcPlay",     MusEGlobal::rcPlayNote);
      xml.intTag(level, "rcSteprec",     MusEGlobal::rcSteprecNote);

      if (writePortInfo) {
            //
            // write information about all midi ports, their assigned
            // instruments and all managed midi controllers
            //
            for (int i = 0; i < MIDI_PORTS; ++i) {
                  bool used = false;
                  MidiPort* mport = &midiPorts[i];
                  MidiDevice* dev = mport->device();
                  // Route check by Tim. Port can now be used for routing even if no device.
                  // Also, check for other non-defaults and save port, to preserve settings even if no device.
                  if(!mport->noInRoute() || !mport->noOutRoute() || 
                  // p4.0.17 Since MidiPort:: and MidiDevice::writeRouting() ignore ports with no device, ignore them here, too.
                  // This prevents bogus routes from being saved and propagated in the med file.
                  // Hmm tough decision, should we save if no device? That would preserve routes in case user upgrades HW, 
                  //  or ALSA reorders or renames devices etc etc, then we have at least kept the track <-> port routes.
                  //if(((!mport->noInRoute() || !mport->noOutRoute()) && dev) || 
                     //mport->defaultInChannels() || mport->defaultOutChannels() ||
                     mport->defaultInChannels() != (1<<MIDI_CHANNELS)-1 ||   // p4.0.17 Default is now to connect to all channels.
                     mport->defaultOutChannels() ||
                     (!mport->instrument()->iname().isEmpty() && mport->instrument()->iname() != "GM") ||
                     !mport->syncInfo().isDefault()) 
                    used = true;  
                  else  
                  {
                    MidiTrackList* tl = song->midis();
                    for (iMidiTrack it = tl->begin(); it != tl->end(); ++it) 
                    {
                      MidiTrack* t = *it;
                      if (t->outPort() == i) 
                      {
                        used = true;
                        break;
                      }
                    }
                  }  
                  
                  if (!used && !dev)
                        continue;
                  xml.tag(level++, "midiport idx=\"%d\"", i);
                  
                  //if(mport->defaultInChannels())
                  if(mport->defaultInChannels() != (1<<MIDI_CHANNELS)-1)     // p4.0.17 Default is now to connect to all channels.
                    xml.intTag(level, "defaultInChans", mport->defaultInChannels());
                  if(mport->defaultOutChannels())
                    xml.intTag(level, "defaultOutChans", mport->defaultOutChannels());
                  
                  if(!mport->instrument()->iname().isEmpty() &&                      // Tim.
                     (mport->instrument()->iname() != "GM"))                         // FIXME: TODO: Make this user configurable.
                    xml.strTag(level, "instrument", mport->instrument()->iname());
                    
                  if (dev) {
                        xml.strTag(level, "name",   dev->name());
                        
                        // p3.3.38
                        //if(dynamic_cast<MidiJackDevice*>(dev))
                        if(dev->deviceType() != MidiDevice::ALSA_MIDI)
                          //xml.intTag(level, "type", MidiDevice::JACK_MIDI);
                          xml.intTag(level, "type", dev->deviceType());
                        
                        // Changed by T356. "record" is old and by mistake written as rwFlags here. 
                        // openFlags was read before, but never written here.
                        //xml.intTag(level, "record", dev->rwFlags() & 0x2 ? 1 : 0);
                        xml.intTag(level, "openFlags", dev->openFlags());
                        }
                  mport->syncInfo().write(level, xml);
                  // write out registered controller for all channels
                  MidiCtrlValListList* vll = mport->controller();
                  for (int k = 0; k < MIDI_CHANNELS; ++k) {
                        int min = k << 24;
                        int max = min + 0x100000;
                        xml.tag(level++, "channel idx=\"%d\"", k);
                        iMidiCtrlValList s = vll->lower_bound(min);
                        iMidiCtrlValList e = vll->lower_bound(max);
                        if (s != e) {
                              for (iMidiCtrlValList i = s; i != e; ++i) {
                                    xml.tag(level++, "controller id=\"%d\"", i->second->num());
                                    if (i->second->hwVal() != CTRL_VAL_UNKNOWN)
                                          xml.intTag(level, "val", i->second->hwVal());
                                    xml.etag(level--, "controller");
                                    }
                              }
                        xml.etag(level--, "channel");
                        }
                  xml.etag(level--, "midiport");
                  }
            }
      xml.tag(level, "/sequencer");
      }

namespace MusEApp {
//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void MusE::writeGlobalConfiguration() const
      {
      FILE* f = fopen(MusEGlobal::configName.toLatin1().constData(), "w");
      if (f == 0) {
            printf("save configuration to <%s> failed: %s\n",
               MusEGlobal::configName.toLatin1().constData(), strerror(errno));
            return;
            }
      Xml xml(f);
      xml.header();
      xml.tag(0, "muse version=\"2.0\"");
      writeGlobalConfiguration(1, xml);
      xml.tag(1, "/muse");
      fclose(f);
      }

void MusE::writeGlobalConfiguration(int level, Xml& xml) const
      {
      xml.tag(level++, "configuration");

      xml.intTag(level, "division", MusEConfig::config.division);
      xml.intTag(level, "rtcTicks", MusEConfig::config.rtcTicks);
      xml.intTag(level, "minMeter", MusEConfig::config.minMeter);
      xml.doubleTag(level, "minSlider", MusEConfig::config.minSlider);
      xml.intTag(level, "freewheelMode", MusEConfig::config.freewheelMode);
      xml.intTag(level, "denormalProtection", MusEConfig::config.useDenormalBias);
      xml.intTag(level, "didYouKnow", MusEConfig::config.showDidYouKnow);
      xml.intTag(level, "outputLimiter", MusEConfig::config.useOutputLimiter);
      xml.intTag(level, "vstInPlace", MusEConfig::config.vstInPlace);
      xml.intTag(level, "dummyAudioBufSize", MusEConfig::config.dummyAudioBufSize);
      xml.intTag(level, "dummyAudioSampleRate", MusEConfig::config.dummyAudioSampleRate);
      xml.uintTag(level, "minControlProcessPeriod", MusEConfig::config.minControlProcessPeriod);

      xml.intTag(level, "guiRefresh", MusEConfig::config.guiRefresh);
      xml.strTag(level, "userInstrumentsDir", MusEConfig::config.userInstrumentsDir);
      // Removed by Orcan. 20101220
      //xml.strTag(level, "helpBrowser", config.helpBrowser);
      xml.intTag(level, "extendedMidi", MusEConfig::config.extendedMidi);
      xml.intTag(level, "midiExportDivision", MusEConfig::config.midiDivision);
      xml.intTag(level, "guiDivision", MusEConfig::config.guiDivision);
      xml.strTag(level, "copyright", MusEConfig::config.copyright);
      xml.intTag(level, "smfFormat", MusEConfig::config.smfFormat);
      xml.intTag(level, "exp2ByteTimeSigs", MusEConfig::config.exp2ByteTimeSigs);
      xml.intTag(level, "expOptimNoteOffs", MusEConfig::config.expOptimNoteOffs);
      xml.intTag(level, "importMidiSplitParts", MusEConfig::config.importMidiSplitParts);
      xml.intTag(level, "startMode", MusEConfig::config.startMode);
      xml.strTag(level, "startSong", MusEConfig::config.startSong);
      xml.strTag(level, "projectBaseFolder", MusEConfig::config.projectBaseFolder);
      xml.intTag(level, "projectStoreInFolder", MusEConfig::config.projectStoreInFolder);
      xml.intTag(level, "useProjectSaveDialog", MusEConfig::config.useProjectSaveDialog);
      xml.intTag(level, "midiInputDevice", MusEGlobal::midiInputPorts);
      xml.intTag(level, "midiInputChannel", MusEGlobal::midiInputChannel);
      xml.intTag(level, "midiRecordType", MusEGlobal::midiRecordType);
      xml.intTag(level, "midiThruType", MusEGlobal::midiThruType);
      xml.intTag(level, "midiFilterCtrl1", MusEGlobal::midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2", MusEGlobal::midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3", MusEGlobal::midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4", MusEGlobal::midiFilterCtrl4);
      
      //xml.intTag(level, "txDeviceId", txDeviceId);
      //xml.intTag(level, "rxDeviceId", rxDeviceId);
      xml.strTag(level, "theme", MusEConfig::config.style);
      xml.strTag(level, "styleSheetFile", MusEConfig::config.styleSheetFile);
      xml.strTag(level, "externalWavEditor", MusEConfig::config.externalWavEditor);
      xml.intTag(level, "useOldStyleStopShortCut", MusEConfig::config.useOldStyleStopShortCut);
      xml.intTag(level, "moveArmedCheckBox", MusEConfig::config.moveArmedCheckBox);
      xml.intTag(level, "popupsDefaultStayOpen", MusEConfig::config.popupsDefaultStayOpen);
      
      //for (int i = 0; i < 6; ++i) {
      for (int i = 0; i < NUM_FONTS; ++i) {
            char buffer[32];
            sprintf(buffer, "font%d", i);
            xml.strTag(level, buffer, MusEConfig::config.fonts[i].toString());
            }
            
      xml.intTag(level, "globalAlphaBlend", MusEConfig::config.globalAlphaBlend);
      
      for (int i = 0; i < 16; ++i) {
            char buffer[32];
            sprintf(buffer, "palette%d", i);
            xml.colorTag(level, buffer, MusEConfig::config.palette[i]);
            }

      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            char buffer[32];
            sprintf(buffer, "partColor%d", i);
            xml.colorTag(level, buffer, MusEConfig::config.partColors[i]);
            }

      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            char buffer[32];
            sprintf(buffer, "partColorName%d", i);
            xml.strTag(level, buffer, MusEConfig::config.partColorNames[i]);
            }

      xml.colorTag(level, "partCanvasBg",  MusEConfig::config.partCanvasBg);
      xml.colorTag(level, "trackBg",       MusEConfig::config.trackBg);
      xml.colorTag(level, "selectTrackBg", MusEConfig::config.selectTrackBg);
      xml.colorTag(level, "selectTrackFg", MusEConfig::config.selectTrackFg);
      
      xml.colorTag(level, "mixerBg",            MusEConfig::config.mixerBg);
      xml.colorTag(level, "midiTrackLabelBg",   MusEConfig::config.midiTrackLabelBg);
      xml.colorTag(level, "drumTrackLabelBg",   MusEConfig::config.drumTrackLabelBg);
      xml.colorTag(level, "waveTrackLabelBg",   MusEConfig::config.waveTrackLabelBg);
      xml.colorTag(level, "outputTrackLabelBg", MusEConfig::config.outputTrackLabelBg);
      xml.colorTag(level, "inputTrackLabelBg",  MusEConfig::config.inputTrackLabelBg);
      xml.colorTag(level, "groupTrackLabelBg",  MusEConfig::config.groupTrackLabelBg);
      xml.colorTag(level, "auxTrackLabelBg",    MusEConfig::config.auxTrackLabelBg);
      xml.colorTag(level, "synthTrackLabelBg",  MusEConfig::config.synthTrackLabelBg);
      
      xml.colorTag(level, "midiTrackBg",   MusEConfig::config.midiTrackBg);
      xml.colorTag(level, "ctrlGraphFg",   MusEConfig::config.ctrlGraphFg);
      xml.colorTag(level, "drumTrackBg",   MusEConfig::config.drumTrackBg);
      xml.colorTag(level, "waveTrackBg",   MusEConfig::config.waveTrackBg);
      xml.colorTag(level, "outputTrackBg", MusEConfig::config.outputTrackBg);
      xml.colorTag(level, "inputTrackBg",  MusEConfig::config.inputTrackBg);
      xml.colorTag(level, "groupTrackBg",  MusEConfig::config.groupTrackBg);
      xml.colorTag(level, "auxTrackBg",    MusEConfig::config.auxTrackBg);
      xml.colorTag(level, "synthTrackBg",  MusEConfig::config.synthTrackBg);

      // Removed by Tim. p3.3.6
      //xml.intTag(level, "txSyncPort", txSyncPort);
      //xml.intTag(level, "rxSyncPort", rxSyncPort);
      xml.intTag(level, "mtctype", mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
        mtcOffset.f(), mtcOffset.sf());
      //xml.uintTag(level, "sendClockDelay", syncSendFirstClockDelay);
      //xml.intTag(level, "useJackTransport", useJackTransport);
      //xml.intTag(level, "jackTransportMaster", jackTransportMaster);
      extSyncFlag.save(level, xml);
      
//      xml.intTag(level, "genMTCSync", genMTCSync);
//      xml.intTag(level, "genMCSync", genMCSync);
//      xml.intTag(level, "genMMC", genMMC);
//      xml.intTag(level, "acceptMTC", acceptMTC);
//      xml.intTag(level, "acceptMMC", acceptMMC);
//      xml.intTag(level, "acceptMC", acceptMC);

      xml.qrectTag(level, "geometryMain",      MusEConfig::config.geometryMain);
      xml.qrectTag(level, "geometryTransport", MusEConfig::config.geometryTransport);
      xml.qrectTag(level, "geometryBigTime",   MusEConfig::config.geometryBigTime);
      xml.qrectTag(level, "geometryPianoroll", MusEConfig::config.geometryPianoroll);
      xml.qrectTag(level, "geometryDrumedit",  MusEConfig::config.geometryDrumedit);
      //xml.qrectTag(level, "geometryMixer",     MusEConfig::config.geometryMixer);  // Obsolete

      xml.intTag(level, "bigtimeVisible", MusEConfig::config.bigTimeVisible);
      xml.intTag(level, "transportVisible", MusEConfig::config.transportVisible);
      
      //xml.intTag(level, "mixerVisible", MusEConfig::config.mixerVisible);  // Obsolete
      xml.intTag(level, "mixer1Visible", MusEConfig::config.mixer1Visible);
      xml.intTag(level, "mixer2Visible", MusEConfig::config.mixer2Visible);
      //MusEConfig::config.mixer1.write(level, xml, "mixer1");
      //MusEConfig::config.mixer2.write(level, xml, "mixer2");
      MusEConfig::config.mixer1.write(level, xml);
      MusEConfig::config.mixer2.write(level, xml);

      xml.intTag(level, "showSplashScreen", MusEConfig::config.showSplashScreen);
      xml.intTag(level, "canvasShowPartType", MusEConfig::config.canvasShowPartType);
      xml.intTag(level, "canvasShowPartEvent", MusEConfig::config.canvasShowPartEvent);
      xml.intTag(level, "canvasShowGrid", MusEConfig::config.canvasShowGrid);
      xml.strTag(level, "canvasBgPixmap", MusEConfig::config.canvasBgPixmap);
      xml.strTag(level, "canvasCustomBgList", MusEConfig::config.canvasCustomBgList.join(";"));

      xml.colorTag(level, "transportHandleColor",  MusEConfig::config.transportHandleColor);
      xml.colorTag(level, "bigtimeForegroundcolor", MusEConfig::config.bigTimeForegroundColor);
      xml.colorTag(level, "bigtimeBackgroundcolor", MusEConfig::config.bigTimeBackgroundColor);
      xml.colorTag(level, "waveEditBackgroundColor", MusEConfig::config.waveEditBackgroundColor);

      writeSeqConfiguration(level, xml, false);

      DrumEdit::writeConfiguration(level, xml);
      PianoRoll::writeConfiguration(level, xml);
      ScoreEdit::write_configuration(level, xml);
      MasterEdit::writeConfiguration(level, xml);
      WaveEdit::writeConfiguration(level, xml);
      ListEdit::writeConfiguration(level, xml);
      ClipListEdit::writeConfiguration(level, xml);
      LMaster::writeConfiguration(level, xml);
      MarkerView::writeConfiguration(level, xml);
      ArrangerView::writeConfiguration(level, xml);
      
      write_function_dialog_config(level, xml);

      writeShortCuts(level, xml);
      xml.etag(level, "configuration");
      }

//---------------------------------------------------------
//   writeConfiguration
//    write song specific configuration
//---------------------------------------------------------

void MusE::writeConfiguration(int level, Xml& xml) const
      {
      xml.tag(level++, "configuration");

      xml.intTag(level, "midiInputDevice",  MusEGlobal::midiInputPorts);
      xml.intTag(level, "midiInputChannel", MusEGlobal::midiInputChannel);
      xml.intTag(level, "midiRecordType",   MusEGlobal::midiRecordType);
      xml.intTag(level, "midiThruType",     MusEGlobal::midiThruType);
      xml.intTag(level, "midiFilterCtrl1",  MusEGlobal::midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2",  MusEGlobal::midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3",  MusEGlobal::midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4",  MusEGlobal::midiFilterCtrl4);

      xml.intTag(level, "waveTracksVisible",  WaveTrack::visible());
      xml.intTag(level, "auxTracksVisible",  AudioAux::visible());
      xml.intTag(level, "groupTracksVisible",  AudioGroup::visible());
      xml.intTag(level, "midiTracksVisible",  MidiTrack::visible());
      xml.intTag(level, "inputTracksVisible",  AudioInput::visible());
      xml.intTag(level, "outputTracksVisible",  AudioOutput::visible());
      xml.intTag(level, "synthTracksVisible",  SynthI::visible());
      // Removed by Tim. p3.3.6
      
      //xml.intTag(level, "txDeviceId", txDeviceId);
      //xml.intTag(level, "rxDeviceId", rxDeviceId);

      // Changed by Tim. p3.3.6
      
      //xml.intTag(level, "txSyncPort", txSyncPort);
      /*
      // To keep old muse versions happy...
      bool mcsync = mmc = mtc = false;
      for(int sp = 0; sp < MIDI_PORTS; ++sp)
      {
        MidiSyncTxPort* txPort = &midiSyncTxPorts[sp];
        if(txPort->doMCSync() || txPort->doMMC() || txPort->doMTC())
        {
          if(txPort->doMCSync())
            mcsync = true;
          if(txPort->doMMC())
            mmc = true;
          if(txPort->doMTC())
            mtc = true;
          xml.intTag(level, "txSyncPort", sp);
          break;
        }  
      }
      */
      
      // Added by Tim. p3.3.6
      
      //xml.tag(level++, "midiSyncInfo");
      //for(iMidiDevice id = midiDevices.begin(); id != midiDevices.end(); ++id) 
      //{
      //  MidiDevice* md = *id;
      //  md->syncInfo().write(level, xml, md);
      //}      
      //xml.etag(level, "midiSyncInfo");

      //xml.intTag(level, "rxSyncPort", rxSyncPort);
      xml.intTag(level, "mtctype", mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
        mtcOffset.f(), mtcOffset.sf());
      xml.uintTag(level, "sendClockDelay", syncSendFirstClockDelay);
      xml.intTag(level, "useJackTransport", useJackTransport.value());
      xml.intTag(level, "jackTransportMaster", jackTransportMaster);
      extSyncFlag.save(level, xml);
      
//      xml.intTag(level, "genMTCSync", genMTCSync);
//      xml.intTag(level, "genMCSync", genMCSync);
//      xml.intTag(level, "genMMC", genMMC);
//      xml.intTag(level, "acceptMTC", acceptMTC);
//      xml.intTag(level, "acceptMMC", acceptMMC);
//      xml.intTag(level, "acceptMC", acceptMC);

      xml.intTag(level, "bigtimeVisible",   viewBigtimeAction->isChecked());
      xml.intTag(level, "transportVisible", viewTransportAction->isChecked());
      xml.intTag(level, "markerVisible",    viewMarkerAction->isChecked());
      //xml.intTag(level, "mixerVisible",     menuView->isItemChecked(aid1));  // Obsolete

      xml.geometryTag(level, "geometryMain", this);
      if (transport)
            xml.geometryTag(level, "geometryTransport", transport);
      if (bigtime)
            xml.geometryTag(level, "geometryBigTime", bigtime);
      
      //if (audioMixer)
      //      xml.geometryTag(level, "geometryMixer", audioMixer);   // Obsolete
      xml.intTag(level, "mixer1Visible",    viewMixerAAction->isChecked());
      xml.intTag(level, "mixer2Visible",    viewMixerBAction->isChecked());
      if (mixer1)
            //mixer1->write(level, xml, "mixer1");
            mixer1->write(level, xml);
      if (mixer2)
            //mixer2->write(level, xml, "mixer2");
            mixer2->write(level, xml);

      _arranger->writeStatus(level, xml);
      writeSeqConfiguration(level, xml, true);

      DrumEdit::writeConfiguration(level, xml);
      PianoRoll::writeConfiguration(level, xml);
      ScoreEdit::write_configuration(level, xml);
      MasterEdit::writeConfiguration(level, xml);
      WaveEdit::writeConfiguration(level, xml);
      
      write_function_dialog_config(level, xml);

      writeMidiTransforms(level, xml);
      writeMidiInputTransforms(level, xml);
      xml.etag(level, "configuration");
      }

//---------------------------------------------------------
//   configMidiSync
//---------------------------------------------------------

void MusE::configMidiSync()
      {
      if (!midiSyncConfig)
        //midiSyncConfig = new MusEWidget::MidiSyncConfig(this);
        midiSyncConfig = new MusEWidget::MidiSyncConfig;
        
      if (midiSyncConfig->isVisible()) {
          midiSyncConfig->raise();
          midiSyncConfig->activateWindow();
          }
      else
            midiSyncConfig->show();
      }

//---------------------------------------------------------
//   configMidiFile
//---------------------------------------------------------

void MusE::configMidiFile()
      {
      if (!midiFileConfig)
            midiFileConfig = new MidiFileConfig();
      midiFileConfig->updateValues();

      if (midiFileConfig->isVisible()) {
          midiFileConfig->raise();
          midiFileConfig->activateWindow();
          }
      else
          midiFileConfig->show();
      }

//---------------------------------------------------------
//   configGlobalSettings
//---------------------------------------------------------

void MusE::configGlobalSettings()
      {
      if (!globalSettingsConfig)
          globalSettingsConfig = new MusEWidget::GlobalSettingsConfig();

      if (globalSettingsConfig->isVisible()) {
          globalSettingsConfig->raise();
          globalSettingsConfig->activateWindow();
          }
      else
          globalSettingsConfig->show();
      }

} // namespace MusEApp

//---------------------------------------------------------
//   MidiFileConfig
//    config properties of exported midi files
//---------------------------------------------------------

MidiFileConfig::MidiFileConfig(QWidget* parent)
  : QDialog(parent), ConfigMidiFileBase()
      {
      setupUi(this);
      connect(buttonOk, SIGNAL(clicked()), SLOT(okClicked()));
      connect(buttonCancel, SIGNAL(clicked()), SLOT(cancelClicked()));
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void MidiFileConfig::updateValues()
      {
      int divisionIdx = 2;
      switch(MusEConfig::config.midiDivision) {
            case 96:  divisionIdx = 0; break;
            case 192:  divisionIdx = 1; break;
            case 384:  divisionIdx = 2; break;
            }
      divisionCombo->setCurrentIndex(divisionIdx);
      formatCombo->setCurrentIndex(MusEConfig::config.smfFormat);
      extendedFormat->setChecked(MusEConfig::config.extendedMidi);
      copyrightEdit->setText(MusEConfig::config.copyright);
      optNoteOffs->setChecked(MusEConfig::config.expOptimNoteOffs);
      twoByteTimeSigs->setChecked(MusEConfig::config.exp2ByteTimeSigs);
      splitPartsCheckBox->setChecked(MusEConfig::config.importMidiSplitParts);
      }

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
      {
      int divisionIdx = divisionCombo->currentIndex();

      int divisions[3] = { 96, 192, 384 };
      if (divisionIdx >= 0 && divisionIdx < 3)
            MusEConfig::config.midiDivision = divisions[divisionIdx];
      MusEConfig::config.extendedMidi = extendedFormat->isChecked();
      MusEConfig::config.smfFormat    = formatCombo->currentIndex();
      MusEConfig::config.copyright    = copyrightEdit->text();
      MusEConfig::config.expOptimNoteOffs = optNoteOffs->isChecked();
      MusEConfig::config.exp2ByteTimeSigs = twoByteTimeSigs->isChecked();
      MusEConfig::config.importMidiSplitParts = splitPartsCheckBox->isChecked();

      MusEGlobal::muse->changeConfig(true);  // write config file
      close();
      }

//---------------------------------------------------------
//   cancelClicked
//---------------------------------------------------------

void MidiFileConfig::cancelClicked()
      {
      close();
      }

namespace MusEConfig {

//---------------------------------------------------------
//   write
//---------------------------------------------------------

//void MixerConfig::write(Xml& xml, const char* name)
void MixerConfig::write(int level, Xml& xml)
//void MixerConfig::write(int level, Xml& xml, const char* name)
      {
      //xml.stag(QString(name));
      //xml.tag(level++, name.toLatin1().constData());
      xml.tag(level++, "Mixer");
      //xml.tag(level++, name);
      
      xml.strTag(level, "name", name);
      
      //xml.tag("geometry",       geometry);
      xml.qrectTag(level, "geometry", geometry);
      
      xml.intTag(level, "showMidiTracks",   showMidiTracks);
      xml.intTag(level, "showDrumTracks",   showDrumTracks);
      xml.intTag(level, "showInputTracks",  showInputTracks);
      xml.intTag(level, "showOutputTracks", showOutputTracks);
      xml.intTag(level, "showWaveTracks",   showWaveTracks);
      xml.intTag(level, "showGroupTracks",  showGroupTracks);
      xml.intTag(level, "showAuxTracks",    showAuxTracks);
      xml.intTag(level, "showSyntiTracks",  showSyntiTracks);
      
      //xml.etag(name);
      //xml.etag(level, name.toLatin1().constData());
      xml.etag(level, "Mixer");
      //xml.etag(level, name);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

//void MixerConfig::read(QDomNode node)
void MixerConfig::read(Xml& xml)
//void MixerConfig::read(Xml& xml, const QString& name)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "name")
                              name = xml.parse1();
                        else if (tag == "geometry")
                              geometry = readGeometry(xml, tag);
                        else if (tag == "showMidiTracks")
                              showMidiTracks = xml.parseInt();
                        else if (tag == "showDrumTracks")
                              showDrumTracks = xml.parseInt();
                        else if (tag == "showInputTracks")
                              showInputTracks = xml.parseInt();
                        else if (tag == "showOutputTracks")
                              showOutputTracks = xml.parseInt();
                        else if (tag == "showWaveTracks")
                              showWaveTracks = xml.parseInt();
                        else if (tag == "showGroupTracks")
                              showGroupTracks = xml.parseInt();
                        else if (tag == "showAuxTracks")
                              showAuxTracks = xml.parseInt();
                        else if (tag == "showSyntiTracks")
                              showSyntiTracks = xml.parseInt();
                        else
                              //xml.unknown(name.toLatin1().constData());
                              xml.unknown("Mixer");
                        break;
                  case Xml::TagEnd:
                        //if (tag == name)
                        if (tag == "Mixer")
                            return;
                  default:
                        break;
                  }
            }
      
      }

} // namespace MusEConfig
