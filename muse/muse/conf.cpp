//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: conf.cpp,v 1.33.2.18 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <sndfile.h>
#include <errno.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <stdio.h>
#include <qpopupmenu.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qsignalmapper.h>
#include <qtooltip.h>
#include <qstyle.h>

#include "app.h"
#include "icons.h"
#include "globals.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "master/masteredit.h"
#include "transport.h"
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
      QString instrument;
      int openFlags = 1;
      bool thruFlag = false;
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
                        else if (tag == "midiSyncInfo")
                              tmpSi.read(xml);
                        else if (tag == "instrument") {
                              instrument = xml.parse1();
                              midiPorts[idx].setInstrument(
                                 registerMidiInstrument(instrument)
                                 );
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
                              
                              //if(debugMsg && !dev)
                              //  fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.latin1());
                                
                              if(!dev && type == MidiDevice::JACK_MIDI)
                              {
                                if(debugMsg)
                                  fprintf(stderr, "readConfigMidiPort: creating jack midi device %s\n", device.latin1());
                                dev = MidiJackDevice::createJackMidiDevice(device, openFlags);
                              }
                              
                              if(debugMsg && !dev)
                                fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.latin1());
                              
                              MidiPort* mp = &midiPorts[idx];
                              mp->syncInfo().copyParams(tmpSi);
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
                            fprintf(stderr, "Read configuration: Sync device: %s not found\n", device.latin1());
                            
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
                              preMeasures = xml.parseInt();
                        else if (tag == "measurepitch")
                              measureClickNote = xml.parseInt();
                        else if (tag == "measurevelo")
                              measureClickVelo = xml.parseInt();
                        else if (tag == "beatpitch")
                              beatClickNote = xml.parseInt();
                        else if (tag == "beatvelo")
                              beatClickVelo = xml.parseInt();
                        else if (tag == "channel")
                              clickChan = xml.parseInt();
                        else if (tag == "port")
                              clickPort = xml.parseInt();
                        else if (tag == "precountEnable")
                              precountEnableFlag = xml.parseInt();
                        else if (tag == "fromMastertrack")
                              precountFromMastertrackFlag = xml.parseInt();
                        else if (tag == "signatureZ")
                              precountSigZ = xml.parseInt();
                        else if (tag == "signatureN")
                              precountSigN = xml.parseInt();
                        else if (tag == "prerecord")
                              precountPrerecord = xml.parseInt();
                        else if (tag == "preroll")
                              precountPreroll = xml.parseInt();
                        else if (tag == "midiClickEnable")
                              midiClickFlag = xml.parseInt();
                        else if (tag == "audioClickEnable")
                              audioClickFlag = xml.parseInt();
                        else if (tag == "audioClickVolume")
                              audioClickVolume = xml.parseFloat();
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
                              rcStopNote = xml.parseInt();
                        else if (tag == "rcEnable")
                              rcEnable = xml.parseInt();
                        else if (tag == "rcRecord")
                              rcRecordNote = xml.parseInt();
                        else if (tag == "rcGotoLeft")
                              rcGotoLeftMarkNote = xml.parseInt();
                        else if (tag == "rcPlay")
                              rcPlayNote = xml.parseInt();
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
                              config.style = xml.parse1();
                        else if (tag == "useOldStyleStopShortCut")
                              config.useOldStyleStopShortCut = xml.parseInt();
                        else if (tag == "externalWavEditor")
                              config.externalWavEditor = xml.parse1();
                        else if (tag == "font0")
                              config.fonts[0].fromString(xml.parse1());
                        else if (tag == "font1")
                              config.fonts[1].fromString(xml.parse1());
                        else if (tag == "font2")
                              config.fonts[2].fromString(xml.parse1());
                        else if (tag == "font3")
                              config.fonts[3].fromString(xml.parse1());
                        else if (tag == "font4")
                              config.fonts[4].fromString(xml.parse1());
                        else if (tag == "font5")
                              config.fonts[5].fromString(xml.parse1());
                        else if (tag == "font6")
                              config.fonts[6].fromString(xml.parse1());
                        else if (tag == "palette0")
                              config.palette[0] = readColor(xml);
                        else if (tag == "palette1")
                              config.palette[1] = readColor(xml);
                        else if (tag == "palette2")
                              config.palette[2] = readColor(xml);
                        else if (tag == "palette3")
                              config.palette[3] = readColor(xml);
                        else if (tag == "palette4")
                              config.palette[4] = readColor(xml);
                        else if (tag == "palette5")
                              config.palette[5] = readColor(xml);
                        else if (tag == "palette6")
                              config.palette[6] = readColor(xml);
                        else if (tag == "palette7")
                              config.palette[7] = readColor(xml);
                        else if (tag == "palette8")
                              config.palette[8] = readColor(xml);
                        else if (tag == "palette9")
                              config.palette[9] = readColor(xml);
                        else if (tag == "palette10")
                              config.palette[10] = readColor(xml);
                        else if (tag == "palette11")
                              config.palette[11] = readColor(xml);
                        else if (tag == "palette12")
                              config.palette[12] = readColor(xml);
                        else if (tag == "palette13")
                              config.palette[13] = readColor(xml);
                        else if (tag == "palette14")
                              config.palette[14] = readColor(xml);
                        else if (tag == "palette15")
                              config.palette[15] = readColor(xml);
                        else if (tag == "palette16")
                              config.palette[16] = readColor(xml);
                        else if (tag == "trackBg")
                              config.trackBg = readColor(xml);
                        else if (tag == "selectTrackBg")
                              config.selectTrackBg = readColor(xml);
                        else if (tag == "selectTrackFg")
                              config.selectTrackFg = readColor(xml);
                        else if (tag == "midiTrackBg")
                              config.midiTrackBg = readColor(xml);
                        else if (tag == "ctrlGraphFg")
                              config.ctrlGraphFg = readColor(xml);
                        else if (tag == "drumTrackBg")
                              config.drumTrackBg = readColor(xml);
                        else if (tag == "waveTrackBg")
                              config.waveTrackBg = readColor(xml);
                        else if (tag == "outputTrackBg")
                              config.outputTrackBg = readColor(xml);
                        else if (tag == "inputTrackBg")
                              config.inputTrackBg = readColor(xml);
                        else if (tag == "groupTrackBg")
                              config.groupTrackBg = readColor(xml);
                        else if (tag == "auxTrackBg")
                              config.auxTrackBg = readColor(xml);
                        else if (tag == "synthTrackBg")
                              config.synthTrackBg = readColor(xml);
                        else if (tag == "extendedMidi")
                              config.extendedMidi = xml.parseInt();
                        else if (tag == "midiExportDivision")
                              config.midiDivision = xml.parseInt();
                        else if (tag == "copyright")
                              config.copyright = xml.parse1();
                        else if (tag == "smfFormat")
                              config.smfFormat = xml.parseInt();
                        else if (tag == "exp2ByteTimeSigs")
                              config.exp2ByteTimeSigs = xml.parseInt();
                        else if (tag == "expOptimNoteOffs")
                              config.expOptimNoteOffs = xml.parseInt();
                        else if (tag == "importMidiSplitParts")
                              config.importMidiSplitParts = xml.parseInt();
                        else if (tag == "midiInputDevice")
                              midiInputPorts = xml.parseInt();
                        else if (tag == "midiInputChannel")
                              midiInputChannel = xml.parseInt();
                        else if (tag == "midiRecordType")
                              midiRecordType = xml.parseInt();
                        else if (tag == "midiThruType")
                              midiThruType = xml.parseInt();
                        else if (tag == "midiFilterCtrl1")
                              midiFilterCtrl1 = xml.parseInt();
                        else if (tag == "midiFilterCtrl2")
                              midiFilterCtrl2 = xml.parseInt();
                        else if (tag == "midiFilterCtrl3")
                              midiFilterCtrl3 = xml.parseInt();
                        else if (tag == "midiFilterCtrl4")
                              midiFilterCtrl4 = xml.parseInt();
                        else if (tag == "bigtimeVisible")
                              config.bigTimeVisible = xml.parseInt();
                        else if (tag == "transportVisible")
                              config.transportVisible = xml.parseInt();
                        else if (tag == "markerVisible")
                              config.markerVisible = xml.parseInt();
                        
                        else if (tag == "mixerVisible")
                              // config.mixerVisible = xml.parseInt();  // Obsolete
                              xml.skip(tag);
                        else if (tag == "mixer1Visible")
                              config.mixer1Visible = xml.parseInt();
                        else if (tag == "mixer2Visible")
                              config.mixer2Visible = xml.parseInt();
                        
                        else if (tag == "showSplashScreen")
                              config.showSplashScreen = xml.parseInt();
                        else if (tag == "canvasShowPartType")
                              config.canvasShowPartType = xml.parseInt();
                        else if (tag == "canvasShowPartEvent")
                              config.canvasShowPartEvent = xml.parseInt();
                        else if (tag == "canvasShowGrid")
                              config.canvasShowGrid = xml.parseInt();
                        else if (tag == "canvasBgPixmap")
                              config.canvasBgPixmap = xml.parse1();
                        else if (tag == "geometryMain")
                              config.geometryMain = readGeometry(xml, tag);
                        else if (tag == "geometryTransport")
                              config.geometryTransport = readGeometry(xml, tag);
                        else if (tag == "geometryBigTime")
                              config.geometryBigTime = readGeometry(xml, tag);
                        else if (tag == "geometryPianoroll")
                              config.geometryPianoroll = readGeometry(xml, tag);
                        else if (tag == "geometryDrumedit")
                              config.geometryDrumedit = readGeometry(xml, tag);
                        
                        else if (tag == "geometryMixer")
                              // config.geometryMixer = readGeometry(xml, tag); // Obsolete
                              xml.skip(tag);
                        //else if (tag == "mixer1")
                        //      config.mixer1.read(xml);
                        //else if (tag == "mixer2")
                        //      config.mixer2.read(xml);
                        else if (tag == "Mixer")
                        {
                              if(mixers == 0)
                                config.mixer1.read(xml);
                              else  
                                config.mixer2.read(xml);
                              ++mixers;
                        }
                        
                        else if (tag == "bigtimeForegroundcolor")
                              config.bigTimeForegroundColor = readColor(xml);
                        else if (tag == "bigtimeBackgroundcolor")
                              config.bigTimeBackgroundColor = readColor(xml);
                        else if (tag == "transportHandleColor")
                              config.transportHandleColor = readColor(xml);
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
                              const char* str = qs.latin1();
                              int h, m, s, f, sf;
                              sscanf(str, "%d:%d:%d:%d:%d", &h, &m, &s, &f, &sf);
                              mtcOffset = MTC(h, m, s, f, sf);
                              }
                        //else if (tag == "midiSyncInfo")
                        //      readConfigMidiSyncInfo(xml);
                        else if (tag == "arranger") {
                              if (muse && muse->arranger)
                                    muse->arranger->readStatus(xml);
                              else
                                    xml.skip(tag);
                              }
                        else if (tag == "drumedit")
                              DrumEdit::readConfiguration(xml);
                        else if (tag == "pianoroll")
                              PianoRoll::readConfiguration(xml);
                        else if (tag == "masteredit")
                              MasterEdit::readConfiguration(xml);
                        else if (tag == "waveedit")
                              WaveEdit::readConfiguration(xml);
                        else if (tag == "shortcuts")
                              readShortCuts(xml);
                        else if (tag == "division")
                              config.division = xml.parseInt();
                        else if (tag == "guiDivision")
                              config.guiDivision = xml.parseInt();
                        else if (tag == "samplerate")
                              xml.parseInt();
                        else if (tag == "segmentsize")
                              xml.parseInt();
                        else if (tag == "segmentcount")
                              xml.parseInt();
                        else if (tag == "rtcTicks")
                              config.rtcTicks = xml.parseInt();
                        else if (tag == "minMeter")
                              config.minMeter = xml.parseInt();
                        else if (tag == "minSlider")
                              config.minSlider = xml.parseDouble();
                        else if (tag == "freewheelMode")
                              config.freewheelMode = xml.parseInt();
                        else if (tag == "denormalProtection")
                              config.useDenormalBias = xml.parseInt();
                        else if (tag == "didYouKnow")
                              config.showDidYouKnow = xml.parseInt();
                        else if (tag == "outputLimiter")
                              config.useOutputLimiter = xml.parseInt();
                        else if (tag == "vstInPlace")
                              config.vstInPlace = xml.parseInt();
                        else if (tag == "dummyAudioSampleRate")
                              config.dummyAudioSampleRate = xml.parseInt();
                        else if (tag == "dummyAudioBufSize")
                              config.dummyAudioBufSize = xml.parseInt();
                        else if (tag == "guiRefresh")
                              config.guiRefresh = xml.parseInt();
                        else if (tag == "helpBrowser")
                              {
                              QString tmp = xml.parse1();
                              if (tmp.isNull()) {tmp = "";}
                              config.helpBrowser = tmp;
                              }
                        else if (tag == "midiTransform")
                              readMidiTransform(xml);
                        else if (tag == "midiInputTransform")
                              readMidiInputTransform(xml);
                        else if (tag == "startMode")
                              config.startMode = xml.parseInt();
                        else if (tag == "startSong")
                              config.startSong = xml.parse1();
                        else
                              xml.unknown("configuration");
                        break;
                  case Xml::Text:
                        printf("text <%s>\n", xml.s1().latin1());
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
//   probeMachineSpecificConfiguration
//---------------------------------------------------------

static void probeMachineSpecificConfiguration()
      {
      // set a default help browser (crude way to find out)
      if (!system("which konqueror > /dev/null"))
           {
           config.helpBrowser = QString("konqueror");
           }
      else if (!system("which opera > /dev/null"))
           {
           config.helpBrowser = QString("opera");
           }
      else if (!system("which mozilla-firefox > /dev/null"))
           {
           config.helpBrowser = QString("mozilla-firefox");
           }
      else if (!system("which firefox > /dev/null"))
           {
           config.helpBrowser = QString("firefox");
           }
      else if (!system("which mozilla > /dev/null"))
           {
           config.helpBrowser = QString("mozilla");
           }
      else
           {
           config.helpBrowser = QString("");
             // was not able to find a browser
           }
      // More preconfiguration
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

bool readConfiguration()
      {
      FILE* f = fopen(configName.latin1(), "r");
      if (f == 0) {
            if (debugMsg || debugMode)
                  fprintf(stderr, "NO Config File <%s> found\n", configName.latin1());

            // if the config file does not exist launch probeMachineSpecificConfiguration
            probeMachineSpecificConfiguration();
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
      xml.intTag(level, "premeasures", preMeasures);
      xml.intTag(level, "measurepitch", measureClickNote);
      xml.intTag(level, "measurevelo", measureClickVelo);
      xml.intTag(level, "beatpitch", beatClickNote);
      xml.intTag(level, "beatvelo", beatClickVelo);
      xml.intTag(level, "channel", clickChan);
      xml.intTag(level, "port", clickPort);

      xml.intTag(level, "precountEnable", precountEnableFlag);
      xml.intTag(level, "fromMastertrack", precountFromMastertrackFlag);
      xml.intTag(level, "signatureZ", precountSigZ);
      xml.intTag(level, "signatureN", precountSigN);
      xml.intTag(level, "prerecord", precountPrerecord);
      xml.intTag(level, "preroll", precountPreroll);
      xml.intTag(level, "midiClickEnable", midiClickFlag);
      xml.intTag(level, "audioClickEnable", audioClickFlag);
      xml.floatTag(level, "audioClickVolume", audioClickVolume);
      xml.tag(level--, "/metronom");

      xml.intTag(level, "rcEnable",   rcEnable);
      xml.intTag(level, "rcStop",     rcStopNote);
      xml.intTag(level, "rcRecord",   rcRecordNote);
      xml.intTag(level, "rcGotoLeft", rcGotoLeftMarkNote);
      xml.intTag(level, "rcPlay",     rcPlayNote);

      if (writePortInfo) {
            //
            // write information about all midi ports, their assigned
            // instruments and all managed midi controllers
            //
            for (int i = 0; i < MIDI_PORTS; ++i) {
                  bool used = false;
                  MidiTrackList* tl = song->midis();
                  for (iMidiTrack it = tl->begin(); it != tl->end(); ++it) {
                        MidiTrack* t = *it;
                        if (t->outPort() == i) {
                              used = true;
                              break;
                              }
                        }
                  MidiPort* mport = &midiPorts[i];
                  MidiDevice* dev = mport->device();
                  if (!used && !dev)
                        continue;
                  xml.tag(level++, "midiport idx=\"%d\"", i);
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

//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void MusE::writeGlobalConfiguration() const
      {
      FILE* f = fopen(configName.latin1(), "w");
      if (f == 0) {
            printf("save configuration to <%s> failed: %s\n",
               configName.latin1(), strerror(errno));
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

      xml.intTag(level, "division", config.division);
      xml.intTag(level, "rtcTicks", config.rtcTicks);
      xml.intTag(level, "minMeter", config.minMeter);
      xml.doubleTag(level, "minSlider", config.minSlider);
      xml.intTag(level, "freewheelMode", config.freewheelMode);
      xml.intTag(level, "denormalProtection", config.useDenormalBias);
      xml.intTag(level, "didYouKnow", config.showDidYouKnow);
      xml.intTag(level, "outputLimiter", config.useOutputLimiter);
      xml.intTag(level, "vstInPlace", config.vstInPlace);
      xml.intTag(level, "dummyAudioBufSize", config.dummyAudioBufSize);
      xml.intTag(level, "dummyAudioSampleRate", config.dummyAudioSampleRate);

      xml.intTag(level, "guiRefresh", config.guiRefresh);
      xml.strTag(level, "helpBrowser", config.helpBrowser);
      xml.intTag(level, "extendedMidi", config.extendedMidi);
      xml.intTag(level, "midiExportDivision", config.midiDivision);
      xml.intTag(level, "guiDivision", config.guiDivision);
      xml.strTag(level, "copyright", config.copyright);
      xml.intTag(level, "smfFormat", config.smfFormat);
      xml.intTag(level, "exp2ByteTimeSigs", config.exp2ByteTimeSigs);
      xml.intTag(level, "expOptimNoteOffs", config.expOptimNoteOffs);
      xml.intTag(level, "importMidiSplitParts", config.importMidiSplitParts);
      xml.intTag(level, "startMode", config.startMode);
      xml.strTag(level, "startSong", config.startSong);

      xml.intTag(level, "midiInputDevice", midiInputPorts);
      xml.intTag(level, "midiInputChannel", midiInputChannel);
      xml.intTag(level, "midiRecordType", midiRecordType);
      xml.intTag(level, "midiThruType", midiThruType);
      xml.intTag(level, "midiFilterCtrl1", midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2", midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3", midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4", midiFilterCtrl4);
      // Removed by Tim. p3.3.6
      
      //xml.intTag(level, "txDeviceId", txDeviceId);
      //xml.intTag(level, "rxDeviceId", rxDeviceId);
      xml.strTag(level, "theme", config.style);
      xml.strTag(level, "externalWavEditor", config.externalWavEditor);
      xml.intTag(level, "useOldStyleStopShortCut", config.useOldStyleStopShortCut);

      //for (int i = 0; i < 6; ++i) {
      for (int i = 0; i < NUM_FONTS; ++i) {
            char buffer[32];
            sprintf(buffer, "font%d", i);
            xml.strTag(level, buffer, config.fonts[i].toString());
            }
      for (int i = 0; i < 16; ++i) {
            char buffer[32];
            sprintf(buffer, "palette%d", i);
            xml.colorTag(level, buffer, config.palette[i]);
            }

      xml.colorTag(level, "trackBg",       config.trackBg);
      xml.colorTag(level, "selectTrackBg", config.selectTrackBg);
      xml.colorTag(level, "selectTrackFg", config.selectTrackFg);
      xml.colorTag(level, "midiTrackBg",   config.midiTrackBg);
      xml.colorTag(level, "ctrlGraphFg",   config.ctrlGraphFg);
      xml.colorTag(level, "drumTrackBg",   config.drumTrackBg);
      xml.colorTag(level, "waveTrackBg",   config.waveTrackBg);
      xml.colorTag(level, "outputTrackBg", config.outputTrackBg);
      xml.colorTag(level, "inputTrackBg",  config.inputTrackBg);
      xml.colorTag(level, "groupTrackBg",  config.groupTrackBg);
      xml.colorTag(level, "auxTrackBg",    config.auxTrackBg);
      xml.colorTag(level, "synthTrackBg",  config.synthTrackBg);

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
      //  (*id)->syncInfo().write(level, xml, md);
      //}      
      //xml.etag(level, "midiSyncInfo");

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

      xml.qrectTag(level, "geometryMain",      config.geometryMain);
      xml.qrectTag(level, "geometryTransport", config.geometryTransport);
      xml.qrectTag(level, "geometryBigTime",   config.geometryBigTime);
      xml.qrectTag(level, "geometryPianoroll", config.geometryPianoroll);
      xml.qrectTag(level, "geometryDrumedit",  config.geometryDrumedit);
      //xml.qrectTag(level, "geometryMixer",     config.geometryMixer);  // Obsolete

      xml.intTag(level, "bigtimeVisible", config.bigTimeVisible);
      xml.intTag(level, "transportVisible", config.transportVisible);
      
      //xml.intTag(level, "mixerVisible", config.mixerVisible);  // Obsolete
      xml.intTag(level, "mixer1Visible", config.mixer1Visible);
      xml.intTag(level, "mixer2Visible", config.mixer2Visible);
      //config.mixer1.write(level, xml, "mixer1");
      //config.mixer2.write(level, xml, "mixer2");
      config.mixer1.write(level, xml);
      config.mixer2.write(level, xml);

      xml.intTag(level, "showSplashScreen", config.showSplashScreen);
      xml.intTag(level, "canvasShowPartType", config.canvasShowPartType);
      xml.intTag(level, "canvasShowPartEvent", config.canvasShowPartEvent);
      xml.intTag(level, "canvasShowGrid", config.canvasShowGrid);
      xml.strTag(level, "canvasBgPixmap", config.canvasBgPixmap);

      xml.colorTag(level, "transportHandleColor",  config.transportHandleColor);
      xml.colorTag(level, "bigtimeForegroundcolor", config.bigTimeForegroundColor);
      xml.colorTag(level, "bigtimeBackgroundcolor", config.bigTimeBackgroundColor);

      writeSeqConfiguration(level, xml, false);

      DrumEdit::writeConfiguration(level, xml);
      PianoRoll::writeConfiguration(level, xml);
      MasterEdit::writeConfiguration(level, xml);
      WaveEdit::writeConfiguration(level, xml);

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

      xml.intTag(level, "midiInputDevice",  midiInputPorts);
      xml.intTag(level, "midiInputChannel", midiInputChannel);
      xml.intTag(level, "midiRecordType",   midiRecordType);
      xml.intTag(level, "midiThruType",     midiThruType);
      xml.intTag(level, "midiFilterCtrl1",  midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2",  midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3",  midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4",  midiFilterCtrl4);
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

      xml.intTag(level, "bigtimeVisible",   menuView->isItemChecked(bt_id));
      xml.intTag(level, "transportVisible", menuView->isItemChecked(tr_id));
      xml.intTag(level, "markerVisible",     menuView->isItemChecked(mr_id));
      //xml.intTag(level, "mixerVisible",     menuView->isItemChecked(aid1));  // Obsolete

      xml.geometryTag(level, "geometryMain", this);
      if (transport)
            xml.geometryTag(level, "geometryTransport", transport);
      if (bigtime)
            xml.geometryTag(level, "geometryBigTime", bigtime);
      
      //if (audioMixer)
      //      xml.geometryTag(level, "geometryMixer", audioMixer);   // Obsolete
      xml.intTag(level, "mixer1Visible",    menuView->isItemChecked(aid1a));
      xml.intTag(level, "mixer2Visible",    menuView->isItemChecked(aid1b));
      if (mixer1)
            //mixer1->write(level, xml, "mixer1");
            mixer1->write(level, xml);
      if (mixer2)
            //mixer2->write(level, xml, "mixer2");
            mixer2->write(level, xml);

      arranger->writeStatus(level, xml);
      writeSeqConfiguration(level, xml, true);

      DrumEdit::writeConfiguration(level, xml);
      PianoRoll::writeConfiguration(level, xml);
      MasterEdit::writeConfiguration(level, xml);
      WaveEdit::writeConfiguration(level, xml);

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
        //midiSyncConfig = new MidiSyncConfig(this);
        midiSyncConfig = new MidiSyncConfig(0, (char*) "midiSyncConfig");
        
      if (midiSyncConfig->isVisible()) {
          midiSyncConfig->raise();
          midiSyncConfig->setActiveWindow();
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
          midiFileConfig->setActiveWindow();
          }
      else
          midiFileConfig->show();
      }

//---------------------------------------------------------
//   MidiFileConfig
//    config properties of exported midi files
//---------------------------------------------------------

MidiFileConfig::MidiFileConfig()
   : ConfigMidiFileBase()
      {
      connect(buttonOk, SIGNAL(clicked()), SLOT(okClicked()));
      connect(buttonCancel, SIGNAL(clicked()), SLOT(cancelClicked()));
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void MidiFileConfig::updateValues()
      {
      int divisionIdx = 2;
      switch(config.midiDivision) {
            case 96:  divisionIdx = 0; break;
            case 192:  divisionIdx = 1; break;
            case 384:  divisionIdx = 2; break;
            }
      divisionCombo->setCurrentItem(divisionIdx);
      formatCombo->setCurrentItem(config.smfFormat);
      extendedFormat->setChecked(config.extendedMidi);
      copyrightEdit->setText(config.copyright);
      optNoteOffs->setChecked(config.expOptimNoteOffs);
      twoByteTimeSigs->setChecked(config.exp2ByteTimeSigs);
      splitPartsCheckBox->setChecked(config.importMidiSplitParts);
      }

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
      {
      int divisionIdx = divisionCombo->currentItem();

      int divisions[3] = { 96, 192, 384 };
      if (divisionIdx >= 0 && divisionIdx < 3)
            config.midiDivision = divisions[divisionIdx];
      config.extendedMidi = extendedFormat->isChecked();
      config.smfFormat    = formatCombo->currentItem();
      config.copyright    = copyrightEdit->text();
      config.expOptimNoteOffs = optNoteOffs->isChecked();
      config.exp2ByteTimeSigs = twoByteTimeSigs->isChecked();
      config.importMidiSplitParts = splitPartsCheckBox->isChecked();

      muse->changeConfig(true);  // write config file
      close();
      }

//---------------------------------------------------------
//   cancelClicked
//---------------------------------------------------------

void MidiFileConfig::cancelClicked()
      {
      close();
      }

//---------------------------------------------------------
//   configGlobalSettings
//---------------------------------------------------------

void MusE::configGlobalSettings()
      {
      if (!globalSettingsConfig)
            globalSettingsConfig = new GlobalSettingsConfig();

      if (globalSettingsConfig->isVisible()) {
          globalSettingsConfig->raise();
          globalSettingsConfig->setActiveWindow();
          }
      else
          globalSettingsConfig->show();
      }


//---------------------------------------------------------
//   write
//---------------------------------------------------------

//void MixerConfig::write(Xml& xml, const char* name)
void MixerConfig::write(int level, Xml& xml)
//void MixerConfig::write(int level, Xml& xml, const char* name)
      {
      //xml.stag(QString(name));
      //xml.tag(level++, name.latin1());
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
      //xml.etag(level, name.latin1());
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
                              //xml.unknown(name.latin1());
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

