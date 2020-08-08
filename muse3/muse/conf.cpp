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

#include <QFile>
#include <QMessageBox>
#include <QString>
#include <QByteArray>

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
#include "bigtime.h"
#include "arranger.h"
#include "conf.h"
#include "gconfig.h"
#include "pitchedit.h"
#include "midiport.h"
#include "mididev.h"
#include "instruments/minstrument.h"
#include "driver/audiodev.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
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
#include "plugin.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "filedialog.h"
#include "al/al.h"

namespace MusECore {

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);

extern void writeMidiInputTransforms(int level, Xml& xml);
extern void readMidiInputTransform(Xml&);

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
                              MidiPort* port = &MusEGlobal::midiPorts[midiPort];
                              val = port->limitValToInstrCtlRange(id, val, channel);
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
//   readConfigMidiDevice
//---------------------------------------------------------

static void readConfigMidiDevice(Xml& xml)
      {
      QString device;
      int rwFlags = 3;
      int openFlags = 1;
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
                        else if (tag == "openFlags")
                              openFlags = xml.parseInt();
                        else if (tag == "rwFlags")             // Jack midi devs need this.
                              rwFlags = xml.parseInt();
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "mididevice") {
                              MidiDevice* dev = MusEGlobal::midiDevices.find(device, type);
                              
                              if(!dev)
                              {
                                if(type == MidiDevice::JACK_MIDI)
                                {
                                  if(MusEGlobal::debugMsg)
                                    fprintf(stderr, "readConfigMidiDevice: creating jack midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                  dev = MidiJackDevice::createJackMidiDevice(device, rwFlags);  
                                }
#ifdef ALSA_SUPPORT
                                else
                                if(type == MidiDevice::ALSA_MIDI)
                                {
                                  if(MusEGlobal::debugMsg)
                                    fprintf(stderr, "readConfigMidiDevice: creating ALSA midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                  dev = MidiAlsaDevice::createAlsaMidiDevice(device, rwFlags);  
                                }
#endif
                              }                              
                              
                              if(MusEGlobal::debugMsg && !dev) 
                                fprintf(stderr, "readConfigMidiDevice: device not found %s\n", device.toLatin1().constData());
                              
                              if (dev) {
                                    dev->setOpenFlags(openFlags);
                                    }
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfigMidiPort
//---------------------------------------------------------

static void readConfigMidiPort(Xml& xml, bool onlyReadChannelState)
      {
      int idx = 0;
      QString device;
              
      // Let's be bold. New users have been confused by generic midi not enabling any patches and controllers.
      // I had said this may cause HW problems by sending out GM sysEx when really the HW might not be GM.
      // But this really needs to be done, one way or another. 
      // FIXME: TODO: Make this user-configurable!
      QString instrument("GM");
      
      int rwFlags = 3;
      int openFlags = 1;
      int dic = -1;   
      int doc = -1;
      int trackIdx = -1;
      
      MidiSyncInfo tmpSi;
      int type = MidiDevice::ALSA_MIDI;
      bool pre_mididevice_ver_found = false;
      
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        
                        // onlyReadChannelState added so it doesn't overwrite midi ports.   p4.0.41 Tim.
                        // Try to keep the controller information. But, this may need to be moved below.  
                        // Also may want to try to keep sync info, but that's a bit risky, so let's not for now.
                        if (tag == "channel") {
                              readPortChannel(xml, idx);
                              break;
                              }
                        else if (onlyReadChannelState){
                              xml.skip(tag);
                              break;
                              }
                              
                        if (tag == "name")
                              device = xml.parse1();
                        else if (tag == "type")
                        {
                              pre_mididevice_ver_found = true;
                              type = xml.parseInt();
                        }
                        else if (tag == "record") {         // old
                              pre_mididevice_ver_found = true;
                              bool f = xml.parseInt();
                              if (f)
                                    openFlags |= 2;
                              }
                        else if (tag == "openFlags")
                        {
                              pre_mididevice_ver_found = true;
                              openFlags = xml.parseInt();
                        }
                        else if (tag == "rwFlags")             // Jack midi devs need this.
                        {
                              pre_mididevice_ver_found = true;
                              rwFlags = xml.parseInt();
                        }
                        else if (tag == "defaultInChans")
                              dic = xml.parseInt(); 
                        else if (tag == "defaultOutChans")
                              doc = xml.parseInt(); 
                        else if (tag == "midiSyncInfo")
                              tmpSi.read(xml);
                        else if (tag == "instrument") {
                              instrument = xml.parse1();
                              //MusEGlobal::midiPorts[idx].setInstrument(    // Moved below
                              //   registerMidiInstrument(instrument)
                              //   );
                              }
                        else if (tag == "trackIdx") {
                              trackIdx = xml.parseInt();
                              }
                        else if (tag == "midithru")
                        {
                              pre_mididevice_ver_found = true;
                              xml.parseInt(); // obsolete
                        }
                        //else if (tag == "channel") {
                        //      readPortChannel(xml, idx);   // Moved above
                        //      }
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
                              
                              if(onlyReadChannelState)      // p4.0.41
                                return;
                              
                              if (idx < 0 || idx >= MusECore::MIDI_PORTS) {
                                    fprintf(stderr, "bad midi port %d (>%d)\n",
                                       idx, MusECore::MIDI_PORTS);
                                    idx = 0;
                                    }
                              
                              MidiDevice* dev = MusEGlobal::midiDevices.find(device, pre_mididevice_ver_found ? type : -1);
                              
                              if(!dev && type == MidiDevice::JACK_MIDI)
                              {
                                if(MusEGlobal::debugMsg)
                                  fprintf(stderr, "readConfigMidiPort: creating jack midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                dev = MidiJackDevice::createJackMidiDevice(device, rwFlags);  
                              }
                              
                              if(MusEGlobal::debugMsg && !dev)
                                fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.toLatin1().constData());
                              
                              MidiPort* mp = &MusEGlobal::midiPorts[idx];

                              mp->setDefaultOutChannels(0); // reset output channel to take care of the case where no default is specified

                              // Just set the generic instrument for now.
                              mp->changeInstrument(genericMidiInstrument);
                              // Set references to be resolved later...
                              mp->setTmpFileRefs(trackIdx, instrument);

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
                                    if(pre_mididevice_ver_found)
                                      dev->setOpenFlags(openFlags);
                                    MusEGlobal::audio->msgSetMidiDevice(mp, dev);
                                    }

                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   loadConfigMetronom
//---------------------------------------------------------

static void loadConfigMetronom(Xml& xml, MetronomeSettings* metro_settings)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "premeasures")
                              metro_settings->preMeasures = xml.parseInt();
                        else if (tag == "measurepitch")
                              metro_settings->measureClickNote = xml.parseInt();
                        else if (tag == "measurevelo")
                              metro_settings->measureClickVelo = xml.parseInt();
                        else if (tag == "beatpitch")
                              metro_settings->beatClickNote = xml.parseInt();
                        else if (tag == "beatvelo")
                              metro_settings->beatClickVelo = xml.parseInt();
                        else if (tag == "accentpitch1")
                              metro_settings->accentClick1 = xml.parseInt();
                        else if (tag == "accentpitch2")
                              metro_settings->accentClick2 = xml.parseInt();
                        else if (tag == "accentvelo1")
                              metro_settings->accentClick1Velo = xml.parseInt();
                        else if (tag == "accentvelo2")
                              metro_settings->accentClick2Velo = xml.parseInt();
                        else if (tag == "channel")
                              metro_settings->clickChan = xml.parseInt();
                        else if (tag == "port")
                              metro_settings->clickPort = xml.parseInt();
                        else if (tag == "precountEnable")
                              metro_settings->precountEnableFlag = xml.parseInt();
                        else if (tag == "fromMastertrack")
                              metro_settings->precountFromMastertrackFlag = xml.parseInt();
                        else if (tag == "signatureZ")
                              metro_settings->precountSigZ = xml.parseInt();
                        else if (tag == "signatureN")
                              metro_settings->precountSigN = xml.parseInt();
                        else if (tag == "precountOnPlay")
                              metro_settings->precountOnPlay = xml.parseInt();
                        else if (tag == "precountMuteMetronome")
                              metro_settings->precountMuteMetronome = xml.parseInt();
                        else if (tag == "prerecord")
                              metro_settings->precountPrerecord = xml.parseInt();
                        else if (tag == "preroll")
                              metro_settings->precountPreroll = xml.parseInt();
                        else if (tag == "midiClickEnable")
                              metro_settings->midiClickFlag = xml.parseInt();
                        else if (tag == "audioClickEnable")
                              metro_settings->audioClickFlag = xml.parseInt();
                        else if (tag == "audioClickVolume")
                              metro_settings->audioClickVolume = xml.parseFloat();
                        else if (tag == "measClickVolume")
                              metro_settings->measClickVolume = xml.parseFloat();
                        else if (tag == "beatClickVolume")
                              metro_settings->beatClickVolume = xml.parseFloat();
                        else if (tag == "accent1ClickVolume")
                              metro_settings->accent1ClickVolume = xml.parseFloat();
                        else if (tag == "accent2ClickVolume")
                              metro_settings->accent2ClickVolume = xml.parseFloat();
                        else if (tag == "clickSamples")
                              metro_settings->clickSamples = (MetronomeSettings::ClickSamples)xml.parseInt();
                        else if (tag == "beatSample")
                              metro_settings->beatSample = xml.parse1();
                        else if (tag == "measSample")
                              metro_settings->measSample = xml.parse1();
                        else if (tag == "accent1Sample")
                              metro_settings->accent1Sample = xml.parse1();
                        else if (tag == "accent2Sample")
                              metro_settings->accent2Sample = xml.parse1();
                        else if (tag == "metroUseSongSettings")
                              MusEGlobal::metroUseSongSettings = xml.parseInt();
                        else if (tag == "metroAccPresets")
                              MusEGlobal::metroAccentPresets.read(xml);
                        else if (tag == "metroAccMap")
                        {
                              if(metro_settings->metroAccentsMap)
                                metro_settings->metroAccentsMap->read(xml);
                        }
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

static void readSeqConfiguration(Xml& xml, MetronomeSettings* metro_settings, bool skipMidiPorts)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "metronom")
                              loadConfigMetronom(xml, metro_settings);
                        else if (tag == "mididevice")
                              readConfigMidiDevice(xml);
                        else if (tag == "midiport")
                              readConfigMidiPort(xml, skipMidiPorts);
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

void readConfiguration(Xml& xml, bool doReadMidiPortConfig, bool doReadGlobalConfig)
      {
      if (doReadGlobalConfig) doReadMidiPortConfig=true;
      
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
                              readSeqConfiguration(xml,
                                doReadGlobalConfig ? &MusEGlobal::metroGlobalSettings : &MusEGlobal::metroSongSettings,
                                !doReadMidiPortConfig);
                              break;
                              }
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
                              MusEGlobal::config.bigTimeVisible = xml.parseInt();
                        else if (tag == "transportVisible")
                              MusEGlobal::config.transportVisible = xml.parseInt();
                        else if (tag == "mixer1Visible")
                              MusEGlobal::config.mixer1Visible = xml.parseInt();
                        else if (tag == "mixer2Visible")
                              MusEGlobal::config.mixer2Visible = xml.parseInt();
                        else if (tag == "markerVisible")
                              // I thought this was obsolete (done by song's toplevel list), but
                              // it's obviously needed. (flo)
                              MusEGlobal::config.markerVisible = xml.parseInt();
                        else if (tag == "arrangerVisible")
                              // same here.
                              MusEGlobal::config.arrangerVisible = xml.parseInt();
                        else if (tag == "geometryTransport")
                              MusEGlobal::config.geometryTransport = readGeometry(xml, tag);
                        else if (tag == "geometryBigTime")
                              MusEGlobal::config.geometryBigTime = readGeometry(xml, tag);
                        else if (tag == "Mixer") {
                              if(mixers == 0)
                                MusEGlobal::config.mixer1.read(xml);
                              else  
                                MusEGlobal::config.mixer2.read(xml);
                              ++mixers;
                              }
                        else if (tag == "geometryMain")
                              MusEGlobal::config.geometryMain = readGeometry(xml, tag);
                        else if (tag == "trackHeight")
                                 MusEGlobal::config.trackHeight = xml.parseInt();


                        else if (doReadMidiPortConfig==false) {
                              xml.skip(tag);
                              break;
                              }
                              
                              
                              
                              
                              
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
                        else if (tag == "mtctype")
                        {
                              MusEGlobal::mtcType= xml.parseInt();
                              // Make sure the AL namespace variables mirror our variables.
                              AL::mtcType = MusEGlobal::mtcType;
                        }
                        else if (tag == "sendClockDelay")
                              MusEGlobal::syncSendFirstClockDelay = xml.parseUInt();
                        else if (tag == "extSync")
                                MusEGlobal::extSyncFlag = xml.parseInt();
                        else if (tag == "useJackTransport")
                                MusEGlobal::config.useJackTransport = xml.parseInt();
                        else if (tag == "timebaseMaster")
                              {
                                MusEGlobal::config.timebaseMaster = xml.parseInt();
                                
                                // Set this one-time flag to true so that when setMaster is called,
                                //  it forces master. audioDevice may be NULL, esp. at startup,
                                //  so this flag is necessary for the next valid call to setMaster.
                                MusEGlobal::timebaseMasterForceFlag = true;
                                if(MusEGlobal::audioDevice)
                                  // Force it.
                                  MusEGlobal::audioDevice->setMaster(MusEGlobal::config.timebaseMaster, true);
                              }  
                        else if (tag == "syncRecFilterPreset")
                              {
                              int p = xml.parseInt();  
                              if(p >= 0 && p < MidiSyncInfo::TYPE_END)
                              {
                                MusEGlobal::syncRecFilterPreset = MidiSyncInfo::SyncRecFilterPresetType(p);
                                MusEGlobal::midiSyncContainer.setSyncRecFilterPreset(MusEGlobal::syncRecFilterPreset);
                              }
                              }
                        else if (tag == "syncRecTempoValQuant")
                              {
                                double qv = xml.parseDouble();
                                MusEGlobal::syncRecTempoValQuant = qv;
                                MusEGlobal::midiSyncContainer.setRecTempoValQuant(qv);
                              }
                        else if (tag == "mtcoffset") {
                              QString qs(xml.parse1());
                              QByteArray ba = qs.toLatin1();
                              const char* str = ba.constData();
                              int h, m, s, f, sf;
                              sscanf(str, "%d:%d:%d:%d:%d", &h, &m, &s, &f, &sf);
                              MusEGlobal::mtcOffset = MTC(h, m, s, f, sf);
                              }
                        else if (tag == "midiTransform")
                              readMidiTransform(xml);
                        else if (tag == "midiInputTransform")
                              readMidiInputTransform(xml);
                              
                        // don't insert else if(...) clauses between
                        // this line and "Global config stuff begins here".
                        else if (!doReadGlobalConfig) {
                              xml.skip(tag);
                              break;
                              }

                        // ---- Global and/or per-song config stuff ends here ----
                        
                        
                        
                        // ---- Global config stuff begins here ----

                        else if (tag == "pluginLadspaPathList")
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginLadspaPathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginLadspaPathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
                        else if (tag == "pluginDssiPathList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginDssiPathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginDssiPathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
                        // Obsolete. Replaced with one below.
                        else if (tag == "pluginVstPathList")
                              xml.parse1();
                        else if (tag == "pluginVstsPathList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginVstPathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginVstPathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
                        // Obsolete. Replaced with one below.
                        else if (tag == "pluginLinuxVstPathList")
                              xml.parse1();
                        else if (tag == "pluginLinuxVstsPathList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginLinuxVstPathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginLinuxVstPathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
                        else if (tag == "pluginLv2PathList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.pluginLv2PathList = xml.parse1().split(":", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.pluginLv2PathList = xml.parse1().split(":", QString::SkipEmptyParts);
#endif
                        else if (tag == "pluginCacheTriggerRescan")
                              MusEGlobal::config.pluginCacheTriggerRescan = xml.parseInt();
                        
                        else if (tag == "audioConverterSettingsGroup")
                        {
                              if(MusEGlobal::defaultAudioConverterSettings)
                                MusEGlobal::defaultAudioConverterSettings->read(xml, &MusEGlobal::audioConverterPluginList);
                        }

                        else if (tag == "preferredRouteNameOrAlias")
                              MusEGlobal::config.preferredRouteNameOrAlias = static_cast<MusEGlobal::RouteNameAliasPreference>(xml.parseInt());
                        else if (tag == "routerExpandVertically")
                              MusEGlobal::config.routerExpandVertically = xml.parseInt();
                        else if (tag == "routerGroupingChannels")
                        {
                              MusEGlobal::config.routerGroupingChannels = xml.parseInt();
                              // TODO: For now we only support maximum two channels grouping. Zero is an error.
                              if(MusEGlobal::config.routerGroupingChannels < 1)
                                MusEGlobal::config.routerGroupingChannels = 1;
                              if(MusEGlobal::config.routerGroupingChannels > 2)
                                MusEGlobal::config.routerGroupingChannels = 2;
                        }
                        else if (tag == "fixFrozenMDISubWindows")
                              MusEGlobal::config.fixFrozenMDISubWindows = xml.parseInt();
                        else if (tag == "theme")
                              MusEGlobal::config.style = xml.parse1();
                        else if (tag == "autoSave")
                              MusEGlobal::config.autoSave = xml.parseInt();
                        else if (tag == "scrollableSubMenus")
                              MusEGlobal::config.scrollableSubMenus = xml.parseInt();
                        else if (tag == "liveWaveUpdate")
                              MusEGlobal::config.liveWaveUpdate = xml.parseInt();
                        else if (tag == "audioEffectsRackVisibleItems")
                              MusEGlobal::config.audioEffectsRackVisibleItems = xml.parseInt();
                        else if (tag == "preferKnobsVsSliders")
                              MusEGlobal::config.preferKnobsVsSliders = xml.parseInt();
                        else if (tag == "showControlValues")
                              MusEGlobal::config.showControlValues = xml.parseInt();
                        else if (tag == "monitorOnRecord")
                              MusEGlobal::config.monitorOnRecord = xml.parseInt();
                        else if (tag == "lineEditStyleHack")
                              MusEGlobal::config.lineEditStyleHack = xml.parseInt();
                        else if (tag == "preferMidiVolumeDb")
                              MusEGlobal::config.preferMidiVolumeDb = xml.parseInt();
                        else if (tag == "midiCtrlGraphMergeErase")
                              MusEGlobal::config.midiCtrlGraphMergeErase = xml.parseInt();
                        else if (tag == "midiCtrlGraphMergeEraseInclusive")
                              MusEGlobal::config.midiCtrlGraphMergeEraseInclusive = xml.parseInt();
                        else if (tag == "midiCtrlGraphMergeEraseWysiwyg")
                              MusEGlobal::config.midiCtrlGraphMergeEraseWysiwyg = xml.parseInt();
                        else if (tag == "styleSheetFile")
                              MusEGlobal::config.styleSheetFile = xml.parse1();
                        else if (tag == "useOldStyleStopShortCut")
                              MusEGlobal::config.useOldStyleStopShortCut = xml.parseInt();
                        else if (tag == "useRewindOnStop")
                              MusEGlobal::config.useRewindOnStop = xml.parseInt();
                        else if (tag == "moveArmedCheckBox")
                              MusEGlobal::config.moveArmedCheckBox = xml.parseInt();
                        else if (tag == "externalWavEditor")
                              MusEGlobal::config.externalWavEditor = xml.parse1();
//                        else if (tag == "font0")
//                              MusEGlobal::config.fonts[0].fromString(xml.parse1());
                        else if (tag == "font1")
                              MusEGlobal::config.fonts[1].fromString(xml.parse1());
                        else if (tag == "font2")
                              MusEGlobal::config.fonts[2].fromString(xml.parse1());
                        else if (tag == "font3")
                              MusEGlobal::config.fonts[3].fromString(xml.parse1());
                        else if (tag == "font4")
                              MusEGlobal::config.fonts[4].fromString(xml.parse1());
                        else if (tag == "font5")
                              MusEGlobal::config.fonts[5].fromString(xml.parse1());
                        else if (tag == "font6")
                              MusEGlobal::config.fonts[6].fromString(xml.parse1());
                        else if (tag == "autoAdjustFontSize")
                              MusEGlobal::config.autoAdjustFontSize = xml.parseInt();
                        else if (tag == "globalAlphaBlend")
                            MusEGlobal::config.globalAlphaBlend = xml.parseInt();
                        else if (tag == "palette0")
                              MusEGlobal::config.palette[0] = readColor(xml);
                        else if (tag == "palette1")
                              MusEGlobal::config.palette[1] = readColor(xml);
                        else if (tag == "palette2")
                              MusEGlobal::config.palette[2] = readColor(xml);
                        else if (tag == "palette3")
                              MusEGlobal::config.palette[3] = readColor(xml);
                        else if (tag == "palette4")
                              MusEGlobal::config.palette[4] = readColor(xml);
                        else if (tag == "palette5")
                              MusEGlobal::config.palette[5] = readColor(xml);
                        else if (tag == "palette6")
                              MusEGlobal::config.palette[6] = readColor(xml);
                        else if (tag == "palette7")
                              MusEGlobal::config.palette[7] = readColor(xml);
                        else if (tag == "palette8")
                              MusEGlobal::config.palette[8] = readColor(xml);
                        else if (tag == "palette9")
                              MusEGlobal::config.palette[9] = readColor(xml);
                        else if (tag == "palette10")
                              MusEGlobal::config.palette[10] = readColor(xml);
                        else if (tag == "palette11")
                              MusEGlobal::config.palette[11] = readColor(xml);
                        else if (tag == "palette12")
                              MusEGlobal::config.palette[12] = readColor(xml);
                        else if (tag == "palette13")
                              MusEGlobal::config.palette[13] = readColor(xml);
                        else if (tag == "palette14")
                              MusEGlobal::config.palette[14] = readColor(xml);
                        else if (tag == "palette15")
                              MusEGlobal::config.palette[15] = readColor(xml);

                        else if (tag == "partColor0")
                              MusEGlobal::config.partColors[0] = readColor(xml);
                        else if (tag == "partColor1")
                              MusEGlobal::config.partColors[1] = readColor(xml);
                        else if (tag == "partColor2")
                              MusEGlobal::config.partColors[2] = readColor(xml);
                        else if (tag == "partColor3")
                              MusEGlobal::config.partColors[3] = readColor(xml);
                        else if (tag == "partColor4")
                              MusEGlobal::config.partColors[4] = readColor(xml);
                        else if (tag == "partColor5")
                              MusEGlobal::config.partColors[5] = readColor(xml);
                        else if (tag == "partColor6")
                              MusEGlobal::config.partColors[6] = readColor(xml);
                        else if (tag == "partColor7")
                              MusEGlobal::config.partColors[7] = readColor(xml);
                        else if (tag == "partColor8")
                              MusEGlobal::config.partColors[8] = readColor(xml);
                        else if (tag == "partColor9")
                              MusEGlobal::config.partColors[9] = readColor(xml);
                        else if (tag == "partColor10")
                              MusEGlobal::config.partColors[10] = readColor(xml);
                        else if (tag == "partColor11")
                              MusEGlobal::config.partColors[11] = readColor(xml);
                        else if (tag == "partColor12")
                              MusEGlobal::config.partColors[12] = readColor(xml);
                        else if (tag == "partColor13")
                              MusEGlobal::config.partColors[13] = readColor(xml);
                        else if (tag == "partColor14")
                              MusEGlobal::config.partColors[14] = readColor(xml);
                        else if (tag == "partColor15")
                              MusEGlobal::config.partColors[15] = readColor(xml);
                        else if (tag == "partColor16")
                              MusEGlobal::config.partColors[16] = readColor(xml);
                        else if (tag == "partColor17")
                              MusEGlobal::config.partColors[17] = readColor(xml);
                        
                        else if (tag == "partColorName0")
                              MusEGlobal::config.partColorNames[0] = xml.parse1();
                        else if (tag == "partColorName1")
                              MusEGlobal::config.partColorNames[1] = xml.parse1();
                        else if (tag == "partColorName2")
                              MusEGlobal::config.partColorNames[2] = xml.parse1();
                        else if (tag == "partColorName3")
                              MusEGlobal::config.partColorNames[3] = xml.parse1();
                        else if (tag == "partColorName4")
                              MusEGlobal::config.partColorNames[4] = xml.parse1();
                        else if (tag == "partColorName5")
                              MusEGlobal::config.partColorNames[5] = xml.parse1();
                        else if (tag == "partColorName6")
                              MusEGlobal::config.partColorNames[6] = xml.parse1();
                        else if (tag == "partColorName7")
                              MusEGlobal::config.partColorNames[7] = xml.parse1();
                        else if (tag == "partColorName8")
                              MusEGlobal::config.partColorNames[8] = xml.parse1();
                        else if (tag == "partColorName9")
                              MusEGlobal::config.partColorNames[9] = xml.parse1();
                        else if (tag == "partColorName10")
                              MusEGlobal::config.partColorNames[10] = xml.parse1();
                        else if (tag == "partColorName11")
                              MusEGlobal::config.partColorNames[11] = xml.parse1();
                        else if (tag == "partColorName12")
                              MusEGlobal::config.partColorNames[12] = xml.parse1();
                        else if (tag == "partColorName13")
                              MusEGlobal::config.partColorNames[13] = xml.parse1();
                        else if (tag == "partColorName14")
                              MusEGlobal::config.partColorNames[14] = xml.parse1();
                        else if (tag == "partColorName15")
                              MusEGlobal::config.partColorNames[15] = xml.parse1();
                        else if (tag == "partColorName16")
                              MusEGlobal::config.partColorNames[16] = xml.parse1();
                        else if (tag == "partColorName17")
                              MusEGlobal::config.partColorNames[17] = xml.parse1();
                        
                        else if (tag == "partCanvasBg")
                              MusEGlobal::config.partCanvasBg = readColor(xml);
                        else if (tag == "dummyPartColor")
                            MusEGlobal::config.dummyPartColor = readColor(xml);
                        else if (tag == "partCanvasFineRaster")
                              MusEGlobal::config.partCanvasFineRasterColor = readColor(xml);
                        else if (tag == "partCanvasCoarseRaster")
                              MusEGlobal::config.partCanvasCoarseRasterColor = readColor(xml);
                        else if (tag == "trackBg")
                              MusEGlobal::config.trackBg = readColor(xml);
                        else if (tag == "selectTrackBg")
                              MusEGlobal::config.selectTrackBg = readColor(xml);
                        else if (tag == "selectTrackFg")
                              MusEGlobal::config.selectTrackFg = readColor(xml);
                        else if (tag == "selectTrackCurBg")
                            MusEGlobal::config.selectTrackCurBg = readColor(xml);
                        else if (tag == "trackSectionDividerColor")
                              MusEGlobal::config.trackSectionDividerColor = readColor(xml);
//                        else if (tag == "mixerBg")
//                              MusEGlobal::config.mixerBg = readColor(xml);
                        else if (tag == "midiTrackLabelBg")
                              MusEGlobal::config.midiTrackLabelBg = readColor(xml);
// Obsolete. There is only 'New' drum tracks now.
                        else if (tag == "drumTrackLabelBg2")
                              /*MusEGlobal::config.drumTrackLabelBg =*/ readColor(xml);
                        else if (tag == "newDrumTrackLabelBg2")
                              MusEGlobal::config.newDrumTrackLabelBg = readColor(xml);
                        else if (tag == "waveTrackLabelBg")
                              MusEGlobal::config.waveTrackLabelBg = readColor(xml);
                        else if (tag == "outputTrackLabelBg")
                              MusEGlobal::config.outputTrackLabelBg = readColor(xml);
                        else if (tag == "inputTrackLabelBg")
                              MusEGlobal::config.inputTrackLabelBg = readColor(xml);
                        else if (tag == "groupTrackLabelBg")
                              MusEGlobal::config.groupTrackLabelBg = readColor(xml);
                        else if (tag == "auxTrackLabelBg2")
                              MusEGlobal::config.auxTrackLabelBg = readColor(xml);
                        else if (tag == "synthTrackLabelBg")
                              MusEGlobal::config.synthTrackLabelBg = readColor(xml);
                        
                        else if (tag == "midiTrackBg")
                              MusEGlobal::config.midiTrackBg = readColor(xml);
                        else if (tag == "ctrlGraphFg")
                              MusEGlobal::config.ctrlGraphFg = readColor(xml);
                        else if (tag == "ctrlGraphSel")
                            MusEGlobal::config.ctrlGraphSel = readColor(xml);
                        else if (tag == "drumTrackBg")
                              MusEGlobal::config.drumTrackBg = readColor(xml);
                        else if (tag == "newDrumTrackBg")
                              MusEGlobal::config.newDrumTrackBg = readColor(xml);
                        else if (tag == "waveTrackBg")
                              MusEGlobal::config.waveTrackBg = readColor(xml);
                        else if (tag == "outputTrackBg")
                              MusEGlobal::config.outputTrackBg = readColor(xml);
                        else if (tag == "inputTrackBg")
                              MusEGlobal::config.inputTrackBg = readColor(xml);
                        else if (tag == "groupTrackBg")
                              MusEGlobal::config.groupTrackBg = readColor(xml);
                        else if (tag == "auxTrackBg")
                              MusEGlobal::config.auxTrackBg = readColor(xml);
                        else if (tag == "synthTrackBg")
                              MusEGlobal::config.synthTrackBg = readColor(xml);

                        else if (tag == "sliderBarDefaultColor")
                              MusEGlobal::config.sliderBarColor = readColor(xml);
                        else if (tag == "sliderDefaultColor2")
                              MusEGlobal::config.sliderBackgroundColor = readColor(xml);
                        else if (tag == "panSliderColor2")
                              MusEGlobal::config.panSliderColor = readColor(xml);
                        else if (tag == "gainSliderColor2")
                              MusEGlobal::config.gainSliderColor = readColor(xml);
                        else if (tag == "auxSliderColor2")
                              MusEGlobal::config.auxSliderColor = readColor(xml);
                        else if (tag == "audioVolumeSliderColor2")
                              MusEGlobal::config.audioVolumeSliderColor = readColor(xml);
                        else if (tag == "midiVolumeSliderColor2")
                              MusEGlobal::config.midiVolumeSliderColor = readColor(xml);
                        else if (tag == "audioVolumeHandleColor")
                            MusEGlobal::config.audioVolumeHandleColor = readColor(xml);
                        else if (tag == "midiVolumeHandleColor")
                            MusEGlobal::config.midiVolumeHandleColor = readColor(xml);
                        else if (tag == "audioControllerSliderDefaultColor2")
                              MusEGlobal::config.audioControllerSliderColor = readColor(xml);
                        else if (tag == "audioPropertySliderDefaultColor2")
                              MusEGlobal::config.audioPropertySliderColor = readColor(xml);
                        else if (tag == "midiControllerSliderDefaultColor2")
                              MusEGlobal::config.midiControllerSliderColor = readColor(xml);
                        else if (tag == "midiPropertySliderDefaultColor2")
                              MusEGlobal::config.midiPropertySliderColor = readColor(xml);
                        else if (tag == "midiPatchReadoutColor")
                              MusEGlobal::config.midiPatchReadoutColor = readColor(xml);
                        else if (tag == "knobFontColor")
                            MusEGlobal::config.knobFontColor = readColor(xml);

                        else if (tag == "audioMeterPrimaryColor")
                              MusEGlobal::config.audioMeterPrimaryColor = readColor(xml);
                        else if (tag == "midiMeterPrimaryColor")
                              MusEGlobal::config.midiMeterPrimaryColor = readColor(xml);
                        else if (tag == "meterBackgroundColor")
                            MusEGlobal::config.meterBackgroundColor = readColor(xml);

                        else if (tag == "rackItemBackgroundColor")
                            MusEGlobal::config.rackItemBackgroundColor = readColor(xml);
                        else if (tag == "rackItemBgActiveColor")
                            MusEGlobal::config.rackItemBgActiveColor = readColor(xml);
                        else if (tag == "rackItemFontColor")
                            MusEGlobal::config.rackItemFontColor = readColor(xml);
                        else if (tag == "rackItemFontActiveColor")
                            MusEGlobal::config.rackItemFontActiveColor = readColor(xml);
                        else if (tag == "rackItemBorderColor")
                            MusEGlobal::config.rackItemBorderColor = readColor(xml);
                        else if (tag == "rackItemFontColorHover")
                            MusEGlobal::config.rackItemFontColorHover = readColor(xml);

                        else if (tag == "palSwitchBackgroundColor")
                            MusEGlobal::config.palSwitchBackgroundColor = readColor(xml);
                        else if (tag == "palSwitchBgActiveColor")
                            MusEGlobal::config.palSwitchBgActiveColor = readColor(xml);
                        else if (tag == "palSwitchFontColor")
                            MusEGlobal::config.palSwitchFontColor = readColor(xml);
                        else if (tag == "palSwitchFontActiveColor")
                            MusEGlobal::config.palSwitchFontActiveColor = readColor(xml);
                        else if (tag == "palSwitchBorderColor")
                            MusEGlobal::config.palSwitchBorderColor = readColor(xml);

                        else if (tag == "midiInstrumentBackgroundColor")
                            MusEGlobal::config.midiInstrumentBackgroundColor = readColor(xml);
                        else if (tag == "midiInstrumentBgActiveColor")
                            MusEGlobal::config.midiInstrumentBgActiveColor = readColor(xml);
                        else if (tag == "midiInstrumentFontColor")
                            MusEGlobal::config.midiInstrumentFontColor = readColor(xml);
                        else if (tag == "midiInstrumentFontActiveColor")
                            MusEGlobal::config.midiInstrumentFontActiveColor = readColor(xml);
                        else if (tag == "midiInstrumentBorderColor")
                            MusEGlobal::config.midiInstrumentBorderColor = readColor(xml);

                        else if (tag == "extendedMidi")
                              MusEGlobal::config.extendedMidi = xml.parseInt();
                        else if (tag == "midiExportDivision")
                              MusEGlobal::config.midiDivision = xml.parseInt();
                        else if (tag == "copyright")
                              MusEGlobal::config.copyright = xml.parse1();
                        else if (tag == "smfFormat")
                              MusEGlobal::config.smfFormat = xml.parseInt();
                        else if (tag == "exp2ByteTimeSigs")
                              MusEGlobal::config.exp2ByteTimeSigs = xml.parseInt();
                        else if (tag == "expOptimNoteOffs")
                              MusEGlobal::config.expOptimNoteOffs = xml.parseInt();
                        else if (tag == "expRunningStatus")
                              MusEGlobal::config.expRunningStatus = xml.parseInt();
                        else if (tag == "importMidiSplitParts")
                              MusEGlobal::config.importMidiSplitParts = xml.parseInt();
                        else if (tag == "importDevNameMetas")
                              MusEGlobal::config.importDevNameMetas = xml.parseInt();
                        else if (tag == "importInstrNameMetas")
                              MusEGlobal::config.importInstrNameMetas = xml.parseInt();
                        else if (tag == "exportPortsDevices")
                              MusEGlobal::config.exportPortsDevices = xml.parseInt();
                        else if (tag == "exportPortDeviceSMF0")
                              MusEGlobal::config.exportPortDeviceSMF0 = xml.parseInt();
                        else if (tag == "exportDrumMapOverrides")
                              MusEGlobal::config.exportDrumMapOverrides = xml.parseInt();
                        else if (tag == "exportChannelOverridesToNewTrack")
                              MusEGlobal::config.exportChannelOverridesToNewTrack = xml.parseInt();
                        else if (tag == "exportModeInstr")
                              MusEGlobal::config.exportModeInstr = xml.parseInt();
                        else if (tag == "importMidiDefaultInstr")
                              MusEGlobal::config.importMidiDefaultInstr = xml.parse1();
                        
                        else if (tag == "showSplashScreen")
                              MusEGlobal::config.showSplashScreen = xml.parseInt();
                        else if (tag == "canvasShowPartType")
                              MusEGlobal::config.canvasShowPartType = xml.parseInt();
                        else if (tag == "canvasShowPartEvent")
                              MusEGlobal::config.canvasShowPartEvent = xml.parseInt();
                        else if (tag == "canvasShowGrid")
                              MusEGlobal::config.canvasShowGrid = xml.parseInt();
                        else if (tag == "canvasBgPixmap")
                              MusEGlobal::config.canvasBgPixmap = xml.parse1();
                        else if (tag == "canvasCustomBgList")
#if QT_VERSION >= 0x050e00
                              MusEGlobal::config.canvasCustomBgList = xml.parse1().split(";", Qt::SkipEmptyParts);
#else
                              MusEGlobal::config.canvasCustomBgList = xml.parse1().split(";", QString::SkipEmptyParts);
#endif
                        else if (tag == "bigtimeForegroundcolor")
                              MusEGlobal::config.bigTimeForegroundColor = readColor(xml);
                        else if (tag == "bigtimeBackgroundcolor")
                              MusEGlobal::config.bigTimeBackgroundColor = readColor(xml);
                        else if (tag == "transportHandleColor")
                              MusEGlobal::config.transportHandleColor = readColor(xml);
                        else if (tag == "waveEditBackgroundColor")
                              MusEGlobal::config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "rulerBackgroundColor")
                              MusEGlobal::config.rulerBg = readColor(xml);
                        else if (tag == "rulerForegroundColor")
                              MusEGlobal::config.rulerFg = readColor(xml);
                        else if (tag == "rulerCurrentColor")
                              MusEGlobal::config.rulerCurrent = readColor(xml);

                        else if (tag == "waveNonselectedPart")
                              MusEGlobal::config.waveNonselectedPart = readColor(xml);
                        else if (tag == "wavePeakColor")
                              MusEGlobal::config.wavePeakColor = readColor(xml);
                        else if (tag == "waveRmsColor")
                              MusEGlobal::config.waveRmsColor = readColor(xml);
                        else if (tag == "wavePeakColorSelected")
                              MusEGlobal::config.wavePeakColorSelected = readColor(xml);
                        else if (tag == "waveRmsColorSelected")
                              MusEGlobal::config.waveRmsColorSelected = readColor(xml);

                        else if (tag == "partWaveColorPeak")
                              MusEGlobal::config.partWaveColorPeak = readColor(xml);
                        else if (tag == "partWaveColorRms")
                              MusEGlobal::config.partWaveColorRms = readColor(xml);
                        else if (tag == "partMidiDarkEventColor")
                              MusEGlobal::config.partMidiDarkEventColor = readColor(xml);
                        else if (tag == "partMidiLightEventColor")
                              MusEGlobal::config.partMidiLightEventColor = readColor(xml);

                        else if (tag == "midiCanvasBackgroundColor")
                              MusEGlobal::config.midiCanvasBg = readColor(xml);

                        else if (tag == "midiCanvasFineColor")
                              MusEGlobal::config.midiCanvasFineColor = readColor(xml);

                        else if (tag == "midiCanvasBeatColor")
                              MusEGlobal::config.midiCanvasBeatColor = readColor(xml);

                        else if (tag == "midiCanvasBarColor")
                              MusEGlobal::config.midiCanvasBarColor = readColor(xml);

                        else if (tag == "midiItemColor")
                            MusEGlobal::config.midiItemColor = readColor(xml);
                        else if (tag == "midiItemSelectedColor")
                            MusEGlobal::config.midiItemSelectedColor = readColor(xml);

                        else if (tag == "midiDividerColor")
                            MusEGlobal::config.midiDividerColor = readColor(xml);

                        else if (tag == "midiControllerViewBackgroundColor")
                              MusEGlobal::config.midiControllerViewBg = readColor(xml);

                        else if (tag == "drumListBackgroundColor")
                              MusEGlobal::config.drumListBg = readColor(xml);
                        else if (tag == "drumListFont")
                            MusEGlobal::config.drumListFont = readColor(xml);
                        else if (tag == "drumListSel")
                            MusEGlobal::config.drumListSel = readColor(xml);
                        else if (tag == "drumListSelFont")
                            MusEGlobal::config.drumListSelFont = readColor(xml);

                        else if (tag == "pianoCurrentKey")
                            MusEGlobal::config.pianoCurrentKey = readColor(xml);
                        else if (tag == "pianoPressedKey")
                            MusEGlobal::config.pianoPressedKey = readColor(xml);
                        else if (tag == "pianoSelectedKey")
                            MusEGlobal::config.pianoSelectedKey = readColor(xml);

                        else if (tag == "maxAliasedPointSize")

                              MusEGlobal::config.maxAliasedPointSize = xml.parseInt();

                        else if (tag == "iconSize")
                            MusEGlobal::config.iconSize = xml.parseInt();

                        else if (tag == "cursorSize")
                            MusEGlobal::config.cursorSize = xml.parseInt();

                        else if (tag == "cascadeStylesheets")
                            MusEGlobal::config.cascadeStylesheets = xml.parseInt();

                        else if (tag == "showIconsInMenus")
                            MusEGlobal::config.showIconsInMenus = xml.parseInt();
                        
                        //else if (tag == "midiSyncInfo")
                        //      readConfigMidiSyncInfo(xml);
                        /* Obsolete. done by song's toplevel list. arrangerview also handles arranger.
                        else if (tag == "arranger") {
                              if (MusEGlobal::muse && MusEGlobal::muse->arranger())
                                    MusEGlobal::muse->arranger()->readStatus(xml);
                              else
                                    xml.skip(tag);
                              }
                        */
                        else if (tag == "drumedit")
                              MusEGui::DrumEdit::readConfiguration(xml);
                        else if (tag == "pianoroll")
                              MusEGui::PianoRoll::readConfiguration(xml);
                        else if (tag == "scoreedit")
                              MusEGui::ScoreEdit::read_configuration(xml);
                        else if (tag == "masteredit")
                              MusEGui::MasterEdit::readConfiguration(xml);
                        else if (tag == "waveedit")
                              MusEGui::WaveEdit::readConfiguration(xml);
                        else if (tag == "listedit")
                              MusEGui::ListEdit::readConfiguration(xml);
                        else if (tag == "cliplistedit")
                              MusEGui::ClipListEdit::readConfiguration(xml);
                        else if (tag == "lmaster")
                              MusEGui::LMaster::readConfiguration(xml);
                        else if (tag == "marker")
                              MusEGui::MarkerView::readConfiguration(xml);
                        else if (tag == "arrangerview")
                              MusEGui::ArrangerView::readConfiguration(xml);
                        
                        else if (tag == "dialogs")
                              MusEGui::read_function_dialog_config(xml);
                        else if (tag == "shortcuts")
                              MusEGui::readShortCuts(xml);
                        else if (tag == "enableAlsaMidiDriver")
                              MusEGlobal::config.enableAlsaMidiDriver = xml.parseInt();
                        else if (tag == "division")
                        {
                              MusEGlobal::config.division = xml.parseInt();
                              // Make sure the AL namespace variable mirrors our variable.
                              AL::division = MusEGlobal::config.division;
                        }
                        else if (tag == "guiDivision")
                              MusEGlobal::config.guiDivision = xml.parseInt();
                        else if (tag == "rtcTicks")
                              MusEGlobal::config.rtcTicks = xml.parseInt();
                        else if (tag == "curMidiSyncInPort")
                              MusEGlobal::config.curMidiSyncInPort = xml.parseInt();
                        else if (tag == "midiSendInit")
                              MusEGlobal::config.midiSendInit = xml.parseInt();
                        else if (tag == "warnInitPending")
                              MusEGlobal::config.warnInitPending = xml.parseInt();
                        else if (tag == "midiSendCtlDefaults")
                              MusEGlobal::config.midiSendCtlDefaults = xml.parseInt();
                        else if (tag == "midiSendNullParameters")
                              MusEGlobal::config.midiSendNullParameters = xml.parseInt();
                        else if (tag == "midiOptimizeControllers")
                              MusEGlobal::config.midiOptimizeControllers = xml.parseInt();
                        else if (tag == "warnIfBadTiming")
                              MusEGlobal::config.warnIfBadTiming = xml.parseInt();
                        else if (tag == "warnOnFileVersions")
                              MusEGlobal::config.warnOnFileVersions = xml.parseInt();
                        else if (tag == "lv2UiBehavior")
                              MusEGlobal::config.lv2UiBehavior = static_cast<MusEGlobal::CONF_LV2_UI_BEHAVIOR>(xml.parseInt());
                        else if (tag == "minMeter")
                              MusEGlobal::config.minMeter = xml.parseInt();
                        else if (tag == "minSlider")
                              MusEGlobal::config.minSlider = xml.parseDouble();
                        else if (tag == "freewheelMode")
                              MusEGlobal::config.freewheelMode = xml.parseInt();
                        else if (tag == "denormalProtection")
                              MusEGlobal::config.useDenormalBias = xml.parseInt();
                        else if (tag == "didYouKnow")
                              MusEGlobal::config.showDidYouKnow = xml.parseInt();
                        else if (tag == "outputLimiter")
                              MusEGlobal::config.useOutputLimiter = xml.parseInt();
                        else if (tag == "vstInPlace")
                              MusEGlobal::config.vstInPlace = xml.parseInt();
                        else if (tag == "deviceAudioSampleRate")
                              MusEGlobal::config.deviceAudioSampleRate = xml.parseInt();
                        else if (tag == "deviceAudioBufSize")
                              MusEGlobal::config.deviceAudioBufSize = xml.parseInt();
                        else if (tag == "deviceAudioBackend")
                              MusEGlobal::config.deviceAudioBackend = xml.parseInt();

                        else if (tag == "enableLatencyCorrection")
                              MusEGlobal::config.enableLatencyCorrection = xml.parseInt();
                        else if (tag == "correctUnterminatedInBranchLatency")
                              MusEGlobal::config.correctUnterminatedInBranchLatency = xml.parseInt();
                        else if (tag == "correctUnterminatedOutBranchLatency")
                              MusEGlobal::config.correctUnterminatedOutBranchLatency = xml.parseInt();
                        else if (tag == "monitoringAffectsLatency")
                              MusEGlobal::config.monitoringAffectsLatency = xml.parseInt();
                        else if (tag == "commonProjectLatency")
                              MusEGlobal::config.commonProjectLatency = xml.parseInt();
                        
                        else if (tag == "minControlProcessPeriod")
                              MusEGlobal::config.minControlProcessPeriod = xml.parseUInt();
                        else if (tag == "guiRefresh")
                              MusEGlobal::config.guiRefresh = xml.parseInt();
                        else if (tag == "userInstrumentsDir")                        // Obsolete
                              MusEGlobal::config.userInstrumentsDir = xml.parse1();  // Keep for compatibility 
                        else if (tag == "startMode")
                              MusEGlobal::config.startMode = xml.parseInt();
                        else if (tag == "startSong")
                              MusEGlobal::config.startSong = xml.parse1();
                        else if (tag == "startSongLoadConfig")
                              MusEGlobal::config.startSongLoadConfig = xml.parseInt();                        
                        else if (tag == "newDrumRecordCondition")
                              MusEGlobal::config.newDrumRecordCondition = MusECore::newDrumRecordCondition_t(xml.parseInt());
                        else if (tag == "projectBaseFolder")
                              MusEGlobal::config.projectBaseFolder = xml.parse1();
                        else if (tag == "projectStoreInFolder")
                              MusEGlobal::config.projectStoreInFolder = xml.parseInt();
                        else if (tag == "useProjectSaveDialog")
                              MusEGlobal::config.useProjectSaveDialog = xml.parseInt();
                        else if (tag == "popupsDefaultStayOpen")
                              MusEGlobal::config.popupsDefaultStayOpen = xml.parseInt();
                        else if (tag == "leftMouseButtonCanDecrease")
                              MusEGlobal::config.leftMouseButtonCanDecrease = xml.parseInt();
                        else if (tag == "rangeMarkersSet")
                              MusEGlobal::config.rangeMarkersSet = (MusEGlobal::CONF_SET_RANGE_MARKERS)xml.parseInt();
//                        else if (tag == "rangeMarkerWithoutMMB")
//                            MusEGlobal::config.rangeMarkerWithoutMMB = xml.parseInt();
                        else if (tag == "addHiddenTracks")
                              MusEGlobal::config.addHiddenTracks = xml.parseInt();
                        else if (tag == "drumTrackPreference")
                              // Obsolete. There is only 'New' drum tracks now.
                              // drumTrackPreference is fixed until it is removed some day...
                              //MusEGlobal::config.drumTrackPreference = (MusEGlobal::drumTrackPreference_t) xml.parseInt();
                              xml.parseInt();

#ifdef _USE_INSTRUMENT_OVERRIDES_
                        else if (tag == "drummapOverrides")
                              MusEGlobal::workingDrumMapInstrumentList.read(xml);
#endif

                        else if (tag == "unhideTracks")
                              MusEGlobal::config.unhideTracks = xml.parseInt();
                        else if (tag == "smartFocus")
                              MusEGlobal::config.smartFocus = xml.parseInt();
                        else if (tag == "borderlessMouse")
                              MusEGlobal::config.borderlessMouse = xml.parseInt();
                        else if (tag == "velocityPerNote")
                              MusEGlobal::config.velocityPerNote = xml.parseInt();
                        else if (tag == "plugin_groups")
                              MusEGlobal::readPluginGroupConfiguration(xml);
                        else if (tag == "mixdownPath")
                              MusEGlobal::config.mixdownPath = xml.parse1();
                        else if (tag == "showNoteNamesInPianoRoll")
                              MusEGlobal::config.showNoteNamesInPianoRoll = xml.parseInt();
                        else if (tag == "showNoteTooltips")
                            MusEGlobal::config.showNoteTooltips = xml.parseInt();
                        else if (tag == "noPluginScaling")
                              MusEGlobal::config.noPluginScaling = xml.parseInt();
                        else if (tag == "openMDIWinMaximized")
                            MusEGlobal::config.openMDIWinMaximized = xml.parseInt();
                        else if (tag == "keepTransportWindowOnTop")
                            MusEGlobal::config.keepTransportWindowOnTop = xml.parseInt();


                        // ---- the following only skips obsolete entries ----
                        else if ((tag == "arranger") || (tag == "geometryPianoroll") || (tag == "geometryDrumedit"))
                              xml.skip(tag);
                        else if (tag == "mixerVisible")
                              xml.skip(tag);
                        else if (tag == "geometryMixer")
                              xml.skip(tag);
                        else if (tag == "txDeviceId")
                                xml.parseInt();
                        else if (tag == "rxDeviceId")
                                xml.parseInt();
                        else if (tag == "txSyncPort")
                                xml.parseInt();
                        else if (tag == "rxSyncPort")
                                xml.parseInt();
                        else if (tag == "syncgentype")
                              xml.parseInt();
                        else if (tag == "genMTCSync")
                              xml.parseInt();
                        else if (tag == "genMCSync")
                              xml.parseInt();
                        else if (tag == "genMMC")
                              xml.parseInt();
                        else if (tag == "acceptMTC")
                              xml.parseInt();
                        else if (tag == "acceptMMC")
                              xml.parseInt();
                        else if (tag == "acceptMC")
                              xml.parseInt();
                        else if ((tag == "samplerate") || (tag == "segmentsize") || (tag == "segmentcount"))
                              xml.parseInt();
                        else
                              xml.unknown("configuration");
                        break;
                  case Xml::Text:
                        fprintf(stderr, "text <%s>\n", xml.s1().toLatin1().constData());
                        break;
                  case Xml::Attribut:
                        if (doReadMidiPortConfig==false)
                              break;
                        else if (tag == "version") {
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
    return readConfiguration(NULL);
}

bool readConfiguration(const char *configFile)
      {
      QByteArray ba;
      if (configFile == NULL)
      {
        ba = MusEGlobal::configName.toLatin1();
        configFile = ba.constData();
      }

      fprintf(stderr, "Config File <%s>\n", configFile);
      FILE* f = fopen(configFile, "r");
      if (f == 0) {
            if (MusEGlobal::debugMsg || MusEGlobal::debugMode)
                  fprintf(stderr, "NO Config File <%s> found\n", configFile);

            if (MusEGlobal::config.userInstrumentsDir.isEmpty()) 
                  MusEGlobal::config.userInstrumentsDir = MusEGlobal::configPath + "/instruments";
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
                        goto read_conf_end;
                  case Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              readConfiguration(xml,true, true /* read global config as well */);
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
                        if(!xml.isVersionEqualToLatest())
                        {
                          fprintf(stderr, "\n***WARNING***\nLoaded config file version is %d.%d\nCurrent version is %d.%d\n"
                                  "Conversions may be applied!\n\n",
                                  xml.majorVersion(), xml.minorVersion(), 
                                  xml.latestMajorVersion(), xml.latestMinorVersion());
                        }
                        if (!skipmode && tag == "muse") {
                              fclose(f);
                              return false;
                              }
                  default:
                        break;
                  }
            }

read_conf_end:
      fclose(f);
      return true;
      }

//---------------------------------------------------------
//   writeMetronomeConfiguration
//---------------------------------------------------------

static void writeMetronomeConfiguration(int level, Xml& xml, bool is_global)
      {
      MusECore::MetronomeSettings* metro_settings = 
        is_global ? &MusEGlobal::metroGlobalSettings : &MusEGlobal::metroSongSettings;

      xml.tag(level++, "metronom");
      xml.intTag(level, "premeasures", metro_settings->preMeasures);
      xml.intTag(level, "measurepitch", metro_settings->measureClickNote);
      xml.intTag(level, "measurevelo", metro_settings->measureClickVelo);
      xml.intTag(level, "beatpitch", metro_settings->beatClickNote);
      xml.intTag(level, "beatvelo", metro_settings->beatClickVelo);
      xml.intTag(level, "accentpitch1", metro_settings->accentClick1);
      xml.intTag(level, "accentpitch2", metro_settings->accentClick2);
      xml.intTag(level, "accentvelo1", metro_settings->accentClick1Velo);
      xml.intTag(level, "accentvelo2", metro_settings->accentClick2Velo);
      xml.intTag(level, "channel", metro_settings->clickChan);
      xml.intTag(level, "port", metro_settings->clickPort);

      // Write the global metroUseSongSettings - ONLY if saving song configuration.
      if(!is_global)
        xml.intTag(level, "metroUseSongSettings", MusEGlobal::metroUseSongSettings);
      // Write either the global or song accents map.
      if(metro_settings->metroAccentsMap)
        metro_settings->metroAccentsMap->write(level, xml);
      // Write the global user accent presets - ONLY if saving global configuration.
      if(is_global)
        MusEGlobal::metroAccentPresets.write(level, xml, MusECore::MetroAccentsStruct::UserPreset);

      xml.intTag(level, "precountEnable", metro_settings->precountEnableFlag);
      xml.intTag(level, "fromMastertrack", metro_settings->precountFromMastertrackFlag);
      xml.intTag(level, "signatureZ", metro_settings->precountSigZ);
      xml.intTag(level, "signatureN", metro_settings->precountSigN);
      xml.intTag(level, "precountOnPlay", metro_settings->precountOnPlay);
      xml.intTag(level, "precountMuteMetronome", metro_settings->precountMuteMetronome);
      xml.intTag(level, "prerecord", metro_settings->precountPrerecord);
      xml.intTag(level, "preroll", metro_settings->precountPreroll);
      xml.intTag(level, "midiClickEnable", metro_settings->midiClickFlag);
      xml.intTag(level, "audioClickEnable", metro_settings->audioClickFlag);
      xml.floatTag(level, "audioClickVolume", metro_settings->audioClickVolume);
      xml.floatTag(level, "measClickVolume", metro_settings->measClickVolume);
      xml.floatTag(level, "beatClickVolume", metro_settings->beatClickVolume);
      xml.floatTag(level, "accent1ClickVolume", metro_settings->accent1ClickVolume);
      xml.floatTag(level, "accent2ClickVolume", metro_settings->accent2ClickVolume);
      xml.intTag(level, "clickSamples", metro_settings->clickSamples);
      xml.strTag(level, "beatSample", metro_settings->beatSample);
      xml.strTag(level, "measSample", metro_settings->measSample);
      xml.strTag(level, "accent1Sample", metro_settings->accent1Sample);
      xml.strTag(level, "accent2Sample", metro_settings->accent2Sample);
      xml.tag(level--, "/metronom");
      }
      
//---------------------------------------------------------
//   writeSeqConfiguration
//---------------------------------------------------------

static void writeSeqConfiguration(int level, Xml& xml, bool writePortInfo)
      {
      xml.tag(level++, "sequencer");

      // If writePortInfo is true we are writing SONG configuration,
      //  and if writePortInfo is NOT true we are writing GLOBAL configuration.
      // Write the global user accent presets - ONLY if saving global configuration.
      writeMetronomeConfiguration(level, xml, !writePortInfo);

      xml.intTag(level, "rcEnable",   MusEGlobal::rcEnable);
      xml.intTag(level, "rcStop",     MusEGlobal::rcStopNote);
      xml.intTag(level, "rcRecord",   MusEGlobal::rcRecordNote);
      xml.intTag(level, "rcGotoLeft", MusEGlobal::rcGotoLeftMarkNote);
      xml.intTag(level, "rcPlay",     MusEGlobal::rcPlayNote);
      xml.intTag(level, "rcSteprec",     MusEGlobal::rcSteprecNote);

      if (writePortInfo) {
            for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
            {
              MidiDevice* dev = *imd;
              // TODO: For now, support only jack midi devices here. ALSA devices are different.
              //if(dev->deviceType() != MidiDevice::JACK_MIDI)
              if(dev->deviceType() != MidiDevice::JACK_MIDI && dev->deviceType() != MidiDevice::ALSA_MIDI)
                continue;

              xml.tag(level++, "mididevice");
              xml.strTag(level, "name",   dev->name());
              
              if(dev->deviceType() != MidiDevice::ALSA_MIDI)
                xml.intTag(level, "type", dev->deviceType());
              
              // Synths will not have been created yet when this is read! So, synthIs now store their own openFlags.
              if(dev->openFlags() != 1)
                xml.intTag(level, "openFlags", dev->openFlags());
              
              if(dev->deviceType() == MidiDevice::JACK_MIDI)
                xml.intTag(level, "rwFlags", dev->rwFlags());   // Need this. Jack midi devs are created by app.   p4.0.41 
              
              xml.etag(level--, "mididevice");
            }
        
            //
            // write information about all midi ports, their assigned
            // instruments and all managed midi controllers
            //
            for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
                  bool used = false;
                  MidiPort* mport = &MusEGlobal::midiPorts[i];
                  MidiDevice* dev = mport->device();
                  // Route check by Tim. Port can now be used for routing even if no device.
                  // Also, check for other non-defaults and save port, to preserve settings even if no device.
                  if(!mport->noInRoute() || !mport->noOutRoute() || 
                  // p4.0.17 Since MidiPort:: and MidiDevice::writeRouting() ignore ports with no device, ignore them here, too.
                  // This prevents bogus routes from being saved and propagated in the med file.
                  // Hmm tough decision, should we save if no device? That would preserve routes in case user upgrades HW, 
                  //  or ALSA reorders or renames devices etc etc, then we have at least kept the track <-> port routes.
                     mport->defaultInChannels() != (1<<MusECore::MUSE_MIDI_CHANNELS)-1 ||   // p4.0.17 Default is now to connect to all channels.
                     mport->defaultOutChannels() ||
                     (!mport->instrument()->iname().isEmpty() && mport->instrument()->midiType() != MT_GM) ||
                     !mport->syncInfo().isDefault()) 
                    used = true;  
                  else  
                  {
                    MidiTrackList* tl = MusEGlobal::song->midis();
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
                  
                  if(mport->defaultInChannels() != (1<<MusECore::MUSE_MIDI_CHANNELS)-1)     // p4.0.17 Default is now to connect to all channels.
                    xml.intTag(level, "defaultInChans", mport->defaultInChannels());
                  if(mport->defaultOutChannels())
                    xml.intTag(level, "defaultOutChans", mport->defaultOutChannels());
                  
                  const MidiInstrument* mi = mport->instrument();
                  // FIXME: TODO: Make this user configurable.
                  if(mi && !mi->iname().isEmpty() && mi->iname() != "GM")
                  {
                    if(mi->isSynti())
                    {
                      // The instrument is a synthesizer. Store a reference to
                      //  the synthesizer track so it can be looked up upon loading.
                      const SynthI* si = static_cast<const SynthI*>(mi);
                      const int idx = MusEGlobal::song->tracks()->index(si);
                      if(idx >= 0)
                        xml.intTag(level, "trackIdx", idx);
                    }
                    else
                    {
                      // The instrument is not a synthesizer, it is one of our own
                      //  (loaded from an *.idf file). Just store a string identifier,
                      //  since we don't have unique indexes for .idf instruments. TODO ???
                      xml.strTag(level, "instrument", mi->iname());
                    }
                  }
                    
                  if (dev) {
                        xml.strTag(level, "name",   dev->name());
                        }
                  mport->syncInfo().write(level, xml);
                  // write out registered controller for all channels
                  MidiCtrlValListList* vll = mport->controller();
                  for (int k = 0; k < MusECore::MUSE_MIDI_CHANNELS; ++k) {
                        int min = k << 24;
                        int max = min + 0x100000;
                        bool found = false;
                        iMidiCtrlValList s = vll->lower_bound(min);
                        iMidiCtrlValList e = vll->lower_bound(max);
                        if (s != e) {
                              for (iMidiCtrlValList i = s; i != e; ++i) {
                                    int ctl = i->second->num();
                                    if(mport->drumController(ctl))  // Including internals like polyaftertouch
                                      ctl |= 0xff;
                                    // Don't bother saving these empty controllers since they are already always added!
                                    if(defaultManagedMidiController.find(ctl) != defaultManagedMidiController.end() 
                                        && i->second->hwVal() == CTRL_VAL_UNKNOWN)
                                      continue;
                                    if(!found)
                                    {
                                      xml.tag(level++, "channel idx=\"%d\"", k);
                                      found = true;
                                    }
                                    xml.tag(level++, "controller id=\"%d\"", i->second->num());
                                    if (i->second->hwVal() != CTRL_VAL_UNKNOWN)
                                          xml.intTag(level, "val", i->second->hwVal());
                                    xml.etag(level--, "controller");
                                    }
                              }
                        if(found)      
                          xml.etag(level--, "channel");
                        }
                  xml.etag(level--, "midiport");
                  }
            }
      xml.tag(level, "/sequencer");
      }
      
      
void writeConfigurationColors(int level, MusECore::Xml& xml, bool partColorNames)
//static void writeConfigurationColors(int level, MusECore::Xml& xml, bool partColorNames = true)
{
     for (int i = 0; i < 16; ++i) {
            xml.colorTag(level, QString("palette") + QString::number(i), MusEGlobal::config.palette[i]);
            }

      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            xml.colorTag(level, QString("partColor") + QString::number(i), MusEGlobal::config.partColors[i]);
            }

      if(partColorNames)
      {
        for (int i = 0; i < NUM_PARTCOLORS; ++i) {
              xml.strTag(level, QString("partColorName") + QString::number(i), MusEGlobal::config.partColorNames[i]);
              }
      }
      
      xml.colorTag(level, "partCanvasBg",  MusEGlobal::config.partCanvasBg);
      xml.colorTag(level, "dummyPartColor",  MusEGlobal::config.dummyPartColor);
      xml.colorTag(level, "partCanvasCoarseRaster",  MusEGlobal::config.partCanvasCoarseRasterColor);
      xml.colorTag(level, "partCanvasFineRaster",  MusEGlobal::config.partCanvasFineRasterColor);

      xml.colorTag(level, "trackBg",       MusEGlobal::config.trackBg);
      xml.colorTag(level, "selectTrackBg", MusEGlobal::config.selectTrackBg);
      xml.colorTag(level, "selectTrackFg", MusEGlobal::config.selectTrackFg);
      xml.colorTag(level, "selectTrackCurBg", MusEGlobal::config.selectTrackCurBg);
      xml.colorTag(level, "trackSectionDividerColor", MusEGlobal::config.trackSectionDividerColor);

//      xml.colorTag(level, "mixerBg",            MusEGlobal::config.mixerBg);
      xml.colorTag(level, "midiTrackLabelBg",   MusEGlobal::config.midiTrackLabelBg);
      xml.colorTag(level, "newDrumTrackLabelBg2",MusEGlobal::config.newDrumTrackLabelBg);
      xml.colorTag(level, "waveTrackLabelBg",   MusEGlobal::config.waveTrackLabelBg);
      xml.colorTag(level, "outputTrackLabelBg", MusEGlobal::config.outputTrackLabelBg);
      xml.colorTag(level, "inputTrackLabelBg",  MusEGlobal::config.inputTrackLabelBg);
      xml.colorTag(level, "groupTrackLabelBg",  MusEGlobal::config.groupTrackLabelBg);
      xml.colorTag(level, "auxTrackLabelBg2",   MusEGlobal::config.auxTrackLabelBg);
      xml.colorTag(level, "synthTrackLabelBg",  MusEGlobal::config.synthTrackLabelBg);
      
      xml.colorTag(level, "midiTrackBg",   MusEGlobal::config.midiTrackBg);
      xml.colorTag(level, "ctrlGraphFg",   MusEGlobal::config.ctrlGraphFg);
      xml.colorTag(level, "ctrlGraphSel",  MusEGlobal::config.ctrlGraphSel);
      xml.colorTag(level, "drumTrackBg",   MusEGlobal::config.drumTrackBg);
      xml.colorTag(level, "newDrumTrackBg",MusEGlobal::config.newDrumTrackBg);
      xml.colorTag(level, "waveTrackBg",   MusEGlobal::config.waveTrackBg);
      xml.colorTag(level, "outputTrackBg", MusEGlobal::config.outputTrackBg);
      xml.colorTag(level, "inputTrackBg",  MusEGlobal::config.inputTrackBg);
      xml.colorTag(level, "groupTrackBg",  MusEGlobal::config.groupTrackBg);
      xml.colorTag(level, "auxTrackBg",    MusEGlobal::config.auxTrackBg);
      xml.colorTag(level, "synthTrackBg",  MusEGlobal::config.synthTrackBg);

      xml.colorTag(level, "sliderBarDefaultColor",  MusEGlobal::config.sliderBarColor);
      xml.colorTag(level, "sliderDefaultColor2",  MusEGlobal::config.sliderBackgroundColor);
      xml.colorTag(level, "panSliderColor2",  MusEGlobal::config.panSliderColor);
      xml.colorTag(level, "gainSliderColor2",  MusEGlobal::config.gainSliderColor);
      xml.colorTag(level, "auxSliderColor2",  MusEGlobal::config.auxSliderColor);
      xml.colorTag(level, "audioVolumeSliderColor2",  MusEGlobal::config.audioVolumeSliderColor);
      xml.colorTag(level, "midiVolumeSliderColor2",  MusEGlobal::config.midiVolumeSliderColor);
      xml.colorTag(level, "audioVolumeHandleColor",  MusEGlobal::config.audioVolumeHandleColor);
      xml.colorTag(level, "midiVolumeHandleColor",  MusEGlobal::config.midiVolumeHandleColor);
      xml.colorTag(level, "audioControllerSliderDefaultColor2",  MusEGlobal::config.audioControllerSliderColor);
      xml.colorTag(level, "audioPropertySliderDefaultColor2",  MusEGlobal::config.audioPropertySliderColor);
      xml.colorTag(level, "midiControllerSliderDefaultColor2",  MusEGlobal::config.midiControllerSliderColor);
      xml.colorTag(level, "midiPropertySliderDefaultColor2",  MusEGlobal::config.midiPropertySliderColor);
      xml.colorTag(level, "midiPatchReadoutColor",  MusEGlobal::config.midiPatchReadoutColor);
      xml.colorTag(level, "knobFontColor",  MusEGlobal::config.knobFontColor);
      xml.colorTag(level, "audioMeterPrimaryColor",  MusEGlobal::config.audioMeterPrimaryColor);
      xml.colorTag(level, "midiMeterPrimaryColor",  MusEGlobal::config.midiMeterPrimaryColor);
      xml.colorTag(level, "meterBackgroundColor",  MusEGlobal::config.meterBackgroundColor);

      xml.colorTag(level, "rackItemBackgroundColor",  MusEGlobal::config.rackItemBackgroundColor);
      xml.colorTag(level, "rackItemBgActiveColor",  MusEGlobal::config.rackItemBgActiveColor);
      xml.colorTag(level, "rackItemFontColor",  MusEGlobal::config.rackItemFontColor);
      xml.colorTag(level, "rackItemFontActiveColor",  MusEGlobal::config.rackItemFontActiveColor);
      xml.colorTag(level, "rackItemBorderColor",  MusEGlobal::config.rackItemBorderColor);
      xml.colorTag(level, "rackItemFontColorHover",  MusEGlobal::config.rackItemFontColorHover);

      xml.colorTag(level, "palSwitchBackgroundColor",  MusEGlobal::config.palSwitchBackgroundColor);
      xml.colorTag(level, "palSwitchBgActiveColor",  MusEGlobal::config.palSwitchBgActiveColor);
      xml.colorTag(level, "palSwitchFontColor",  MusEGlobal::config.palSwitchFontColor);
      xml.colorTag(level, "palSwitchFontActiveColor",  MusEGlobal::config.palSwitchFontActiveColor);
      xml.colorTag(level, "palSwitchBorderColor",  MusEGlobal::config.palSwitchBorderColor);

      xml.colorTag(level, "midiInstrumentBackgroundColor",  MusEGlobal::config.midiInstrumentBackgroundColor);
      xml.colorTag(level, "midiInstrumentBgActiveColor",  MusEGlobal::config.midiInstrumentBgActiveColor);
      xml.colorTag(level, "midiInstrumentFontColor",  MusEGlobal::config.midiInstrumentFontColor);
      xml.colorTag(level, "midiInstrumentFontActiveColor",  MusEGlobal::config.midiInstrumentFontActiveColor);
      xml.colorTag(level, "midiInstrumentBorderColor",  MusEGlobal::config.midiInstrumentBorderColor);

      xml.colorTag(level, "transportHandleColor",  MusEGlobal::config.transportHandleColor);
      xml.colorTag(level, "bigtimeForegroundcolor", MusEGlobal::config.bigTimeForegroundColor);
      xml.colorTag(level, "bigtimeBackgroundcolor", MusEGlobal::config.bigTimeBackgroundColor);
      xml.colorTag(level, "waveEditBackgroundColor", MusEGlobal::config.waveEditBackgroundColor);
      xml.colorTag(level, "rulerBackgroundColor", MusEGlobal::config.rulerBg);
      xml.colorTag(level, "rulerForegroundColor", MusEGlobal::config.rulerFg);
      xml.colorTag(level, "rulerCurrentColor", MusEGlobal::config.rulerCurrent);

      xml.colorTag(level, "waveNonselectedPart", MusEGlobal::config.waveNonselectedPart);
      xml.colorTag(level, "wavePeakColor", MusEGlobal::config.wavePeakColor);
      xml.colorTag(level, "waveRmsColor", MusEGlobal::config.waveRmsColor);
      xml.colorTag(level, "wavePeakColorSelected", MusEGlobal::config.wavePeakColorSelected);
      xml.colorTag(level, "waveRmsColorSelected", MusEGlobal::config.waveRmsColorSelected);

      xml.colorTag(level, "partWaveColorPeak", MusEGlobal::config.partWaveColorPeak);
      xml.colorTag(level, "partWaveColorRms", MusEGlobal::config.partWaveColorRms);
      xml.colorTag(level, "partMidiDarkEventColor", MusEGlobal::config.partMidiDarkEventColor);
      xml.colorTag(level, "partMidiLightEventColor", MusEGlobal::config.partMidiLightEventColor);

      xml.colorTag(level, "midiCanvasBackgroundColor", MusEGlobal::config.midiCanvasBg);
      xml.colorTag(level, "midiCanvasFineColor", MusEGlobal::config.midiCanvasFineColor);
      xml.colorTag(level, "midiCanvasBeatColor", MusEGlobal::config.midiCanvasBeatColor);
      xml.colorTag(level, "midiCanvasBarColor", MusEGlobal::config.midiCanvasBarColor);
      xml.colorTag(level, "midiDividerColor", MusEGlobal::config.midiDividerColor);

      xml.colorTag(level, "midiItemColor", MusEGlobal::config.midiItemColor);
      xml.colorTag(level, "midiItemSelectedColor", MusEGlobal::config.midiItemSelectedColor);

      xml.colorTag(level, "midiControllerViewBackgroundColor", MusEGlobal::config.midiControllerViewBg);
      xml.colorTag(level, "drumListBackgroundColor", MusEGlobal::config.drumListBg);
      xml.colorTag(level, "drumListFont", MusEGlobal::config.drumListFont);
      xml.colorTag(level, "drumListSel", MusEGlobal::config.drumListSel);
      xml.colorTag(level, "drumListSelFont", MusEGlobal::config.drumListSelFont);

      xml.colorTag(level, "pianoCurrentKey", MusEGlobal::config.pianoCurrentKey);
      xml.colorTag(level, "pianoPressedKey", MusEGlobal::config.pianoPressedKey);
      xml.colorTag(level, "pianoSelectedKey", MusEGlobal::config.pianoSelectedKey);
}
      

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void MusE::writeGlobalConfiguration() const
      {
      FILE* f = fopen(MusEGlobal::configName.toLatin1().constData(), "w");
      if (f == 0) {
            fprintf(stderr, "save configuration to <%s> failed: %s\n",
               MusEGlobal::configName.toLatin1().constData(), strerror(errno));
            return;
            }
      MusECore::Xml xml(f);
      xml.header();
      xml.nput(0, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
      writeGlobalConfiguration(1, xml);
      xml.tag(1, "/muse");
      fclose(f);
      }

bool MusE::loadConfigurationColors(QWidget* parent)
{
  if(!parent)
    parent = this;
  //QString file = QFileDialog::getOpenFileName(parent, tr("Load configuration colors"), QString(), tr("MusE color configuration files *.cfc (*.cfc)"));
  QString file = MusEGui::getOpenFileName(QString("themes"), MusEGlobal::colors_config_file_pattern, this,
                                               tr("Load configuration colors"), NULL, MusEGui::MFileDialog::GLOBAL_VIEW);

  if(file.isEmpty())
    return false;
  
  if(QMessageBox::question(parent, QString("MusE"),
      tr("Color settings will immediately be replaced with any found in the file.\nAre you sure you want to proceed?"), tr("&Ok"), tr("&Cancel"),
      QString(), 0, 1 ) == 1)
    return false;
  
  // Read, and return if error.
  if(MusECore::readConfiguration(file.toLatin1().constData()))   // True if error.
  {
    fprintf(stderr, "MusE::loadConfigurationColors failed\n");
    return false;
  }
  // Notify app, and write into configuration file.
  // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
  changeConfig(true);
  return true;
}

bool MusE::saveConfigurationColors(QWidget* parent)
{
  if(!parent)
    parent = this;
  QString file = MusEGui::getSaveFileName(QString("themes"), MusEGlobal::colors_config_file_pattern, this,
                                               tr("Save configuration colors"), NULL, MusEGui::MFileDialog::USER_VIEW);

  if(file.isEmpty())
    return false;

// redundant, this is already done by the file dialog itself (kybos)
//  if(QFile::exists(file))
//  {
//    if(QMessageBox::question(parent, QString("MusE"),
//        tr("File exists.\nDo you want to overwrite it?"), tr("&Ok"), tr("&Cancel"),
//        QString(), 0, 1 ) == 1)
//      return false;
//  }

  FILE* f = fopen(file.toLatin1().constData(), "w");
  if (f == 0) 
  {
    fprintf(stderr, "save configuration colors to <%s> failed: %s\n",
        file.toLatin1().constData(), strerror(errno));
    return false;
  }
  MusECore::Xml xml(f);
  xml.header();
  xml.nput(0, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
  xml.tag(1, "configuration");
  MusECore::writeConfigurationColors(2, xml, false); // Don't save part colour names.
  xml.etag(1, "configuration");
  xml.tag(0, "/muse");
  fclose(f);
  return true;
}
      
void MusE::writeGlobalConfiguration(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "configuration");

      xml.strTag(level, "pluginLadspaPathList", MusEGlobal::config.pluginLadspaPathList.join(":"));
      xml.strTag(level, "pluginDssiPathList", MusEGlobal::config.pluginDssiPathList.join(":"));
      xml.strTag(level, "pluginVstsPathList", MusEGlobal::config.pluginVstPathList.join(":"));
      xml.strTag(level, "pluginLinuxVstsPathList", MusEGlobal::config.pluginLinuxVstPathList.join(":"));
      xml.strTag(level, "pluginLv2PathList", MusEGlobal::config.pluginLv2PathList.join(":"));
      
      if(MusEGlobal::defaultAudioConverterSettings)
        MusEGlobal::defaultAudioConverterSettings->write(level, xml, &MusEGlobal::audioConverterPluginList);
      
      xml.intTag(level, "pluginCacheTriggerRescan", MusEGlobal::config.pluginCacheTriggerRescan);
                        
      xml.intTag(level, "enableAlsaMidiDriver", MusEGlobal::config.enableAlsaMidiDriver);
      xml.intTag(level, "division", MusEGlobal::config.division);
      xml.intTag(level, "rtcTicks", MusEGlobal::config.rtcTicks);
      xml.intTag(level, "curMidiSyncInPort", MusEGlobal::config.curMidiSyncInPort);
      xml.intTag(level, "midiSendInit", MusEGlobal::config.midiSendInit);
      xml.intTag(level, "warnInitPending", MusEGlobal::config.warnInitPending);
      xml.intTag(level, "midiSendCtlDefaults", MusEGlobal::config.midiSendCtlDefaults);
      xml.intTag(level, "midiSendNullParameters", MusEGlobal::config.midiSendNullParameters);
      xml.intTag(level, "midiOptimizeControllers", MusEGlobal::config.midiOptimizeControllers);
      xml.intTag(level, "warnIfBadTiming", MusEGlobal::config.warnIfBadTiming);
      xml.intTag(level, "warnOnFileVersions", MusEGlobal::config.warnOnFileVersions);
      xml.intTag(level, "minMeter", MusEGlobal::config.minMeter);
      xml.doubleTag(level, "minSlider", MusEGlobal::config.minSlider);
      xml.intTag(level, "freewheelMode", MusEGlobal::config.freewheelMode);
      xml.intTag(level, "denormalProtection", MusEGlobal::config.useDenormalBias);
      xml.intTag(level, "didYouKnow", MusEGlobal::config.showDidYouKnow);
      xml.intTag(level, "outputLimiter", MusEGlobal::config.useOutputLimiter);
      xml.intTag(level, "vstInPlace", MusEGlobal::config.vstInPlace);

      xml.intTag(level, "deviceAudioBufSize", MusEGlobal::config.deviceAudioBufSize);
      xml.intTag(level, "deviceAudioSampleRate", MusEGlobal::config.deviceAudioSampleRate);
      xml.intTag(level, "deviceAudioBackend", MusEGlobal::config.deviceAudioBackend);

      xml.intTag(level, "enableLatencyCorrection", MusEGlobal::config.enableLatencyCorrection);
      xml.intTag(level, "correctUnterminatedInBranchLatency", MusEGlobal::config.correctUnterminatedInBranchLatency);
      xml.intTag(level, "correctUnterminatedOutBranchLatency", MusEGlobal::config.correctUnterminatedOutBranchLatency);
      xml.intTag(level, "monitoringAffectsLatency", MusEGlobal::config.monitoringAffectsLatency);
      xml.intTag(level, "commonProjectLatency", MusEGlobal::config.commonProjectLatency);


      xml.uintTag(level, "minControlProcessPeriod", MusEGlobal::config.minControlProcessPeriod);
      xml.intTag(level, "guiRefresh", MusEGlobal::config.guiRefresh);
      
      xml.intTag(level, "extendedMidi", MusEGlobal::config.extendedMidi);
      xml.intTag(level, "midiExportDivision", MusEGlobal::config.midiDivision);
      xml.intTag(level, "guiDivision", MusEGlobal::config.guiDivision);
      xml.strTag(level, "copyright", MusEGlobal::config.copyright);
      xml.intTag(level, "smfFormat", MusEGlobal::config.smfFormat);
      xml.intTag(level, "expRunningStatus", MusEGlobal::config.expRunningStatus);
      xml.intTag(level, "exp2ByteTimeSigs", MusEGlobal::config.exp2ByteTimeSigs);
      xml.intTag(level, "expOptimNoteOffs", MusEGlobal::config.expOptimNoteOffs);
      xml.intTag(level, "importMidiSplitParts", MusEGlobal::config.importMidiSplitParts);
      xml.intTag(level, "importDevNameMetas", MusEGlobal::config.importDevNameMetas);
      xml.intTag(level, "importInstrNameMetas", MusEGlobal::config.importInstrNameMetas);
      xml.intTag(level, "exportPortsDevices", MusEGlobal::config.exportPortsDevices);
      xml.intTag(level, "exportPortDeviceSMF0", MusEGlobal::config.exportPortDeviceSMF0);
      xml.intTag(level, "exportDrumMapOverrides", MusEGlobal::config.exportDrumMapOverrides);
      xml.intTag(level, "exportChannelOverridesToNewTrack", MusEGlobal::config.exportChannelOverridesToNewTrack);
      xml.intTag(level, "exportModeInstr", MusEGlobal::config.exportModeInstr);
      xml.strTag(level, "importMidiDefaultInstr", MusEGlobal::config.importMidiDefaultInstr);
      xml.intTag(level, "startMode", MusEGlobal::config.startMode);
      xml.strTag(level, "startSong", MusEGlobal::config.startSong);
      xml.intTag(level, "startSongLoadConfig", MusEGlobal::config.startSongLoadConfig);
      xml.intTag(level, "newDrumRecordCondition", MusEGlobal::config.newDrumRecordCondition);
      xml.strTag(level, "projectBaseFolder", MusEGlobal::config.projectBaseFolder);
      xml.intTag(level, "projectStoreInFolder", MusEGlobal::config.projectStoreInFolder);
      xml.intTag(level, "useProjectSaveDialog", MusEGlobal::config.useProjectSaveDialog);
      xml.intTag(level, "midiInputDevice", MusEGlobal::midiInputPorts);
      xml.intTag(level, "midiInputChannel", MusEGlobal::midiInputChannel);
      xml.intTag(level, "midiRecordType", MusEGlobal::midiRecordType);
      xml.intTag(level, "midiThruType", MusEGlobal::midiThruType);
      xml.intTag(level, "midiFilterCtrl1", MusEGlobal::midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2", MusEGlobal::midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3", MusEGlobal::midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4", MusEGlobal::midiFilterCtrl4);
      
      xml.intTag(level, "preferredRouteNameOrAlias", static_cast<int>(MusEGlobal::config.preferredRouteNameOrAlias));
      xml.intTag(level, "routerExpandVertically", MusEGlobal::config.routerExpandVertically);
      xml.intTag(level, "routerGroupingChannels", MusEGlobal::config.routerGroupingChannels);
      
      xml.intTag(level, "fixFrozenMDISubWindows", MusEGlobal::config.fixFrozenMDISubWindows);
      xml.strTag(level, "theme", MusEGlobal::config.style);
      xml.intTag(level, "autoSave", MusEGlobal::config.autoSave);
      xml.strTag(level, "styleSheetFile", MusEGlobal::config.styleSheetFile);
      xml.strTag(level, "externalWavEditor", MusEGlobal::config.externalWavEditor);
      xml.intTag(level, "useOldStyleStopShortCut", MusEGlobal::config.useOldStyleStopShortCut);
      xml.intTag(level, "useRewindOnStop", MusEGlobal::config.useRewindOnStop);
      xml.intTag(level, "moveArmedCheckBox", MusEGlobal::config.moveArmedCheckBox);
      xml.intTag(level, "popupsDefaultStayOpen", MusEGlobal::config.popupsDefaultStayOpen);
      xml.intTag(level, "leftMouseButtonCanDecrease", MusEGlobal::config.leftMouseButtonCanDecrease);
      xml.intTag(level, "rangeMarkersSet", MusEGlobal::config.rangeMarkersSet);
//      xml.intTag(level, "rangeMarkerWithoutMMB", MusEGlobal::config.rangeMarkerWithoutMMB);
      xml.intTag(level, "smartFocus", MusEGlobal::config.smartFocus);
      xml.intTag(level, "borderlessMouse", MusEGlobal::config.borderlessMouse);
      xml.intTag(level, "velocityPerNote", MusEGlobal::config.velocityPerNote);
      
      xml.intTag(level, "unhideTracks", MusEGlobal::config.unhideTracks);
      xml.intTag(level, "addHiddenTracks", MusEGlobal::config.addHiddenTracks);
      // Obsolete. There is only 'New' drum tracks now.
      // drumTrackPreference is fixed until it is removed some day...
      //xml.intTag(level, "drumTrackPreference", MusEGlobal::config.drumTrackPreference);

#ifdef _USE_INSTRUMENT_OVERRIDES_
      MusECore::midiInstruments.writeDrummapOverrides(level, xml);
#endif

      xml.intTag(level, "waveTracksVisible",  MusECore::WaveTrack::visible());
      xml.intTag(level, "auxTracksVisible",  MusECore::AudioAux::visible());
      xml.intTag(level, "groupTracksVisible",  MusECore::AudioGroup::visible());
      xml.intTag(level, "midiTracksVisible",  MusECore::MidiTrack::visible());
      xml.intTag(level, "inputTracksVisible",  MusECore::AudioInput::visible());
      xml.intTag(level, "outputTracksVisible",  MusECore::AudioOutput::visible());
      xml.intTag(level, "synthTracksVisible",  MusECore::SynthI::visible());
      xml.intTag(level, "trackHeight",  MusEGlobal::config.trackHeight);
      xml.intTag(level, "scrollableSubMenus", MusEGlobal::config.scrollableSubMenus);
      xml.intTag(level, "liveWaveUpdate", MusEGlobal::config.liveWaveUpdate);
      xml.intTag(level, "audioEffectsRackVisibleItems", MusEGlobal::config.audioEffectsRackVisibleItems);
      xml.intTag(level, "preferKnobsVsSliders", MusEGlobal::config.preferKnobsVsSliders);
      xml.intTag(level, "showControlValues", MusEGlobal::config.showControlValues);
      xml.intTag(level, "monitorOnRecord", MusEGlobal::config.monitorOnRecord);
      xml.intTag(level, "lineEditStyleHack", MusEGlobal::config.lineEditStyleHack);
      xml.intTag(level, "preferMidiVolumeDb", MusEGlobal::config.preferMidiVolumeDb);
      xml.intTag(level, "midiCtrlGraphMergeErase", MusEGlobal::config.midiCtrlGraphMergeErase);
      xml.intTag(level, "midiCtrlGraphMergeEraseInclusive", MusEGlobal::config.midiCtrlGraphMergeEraseInclusive);
      xml.intTag(level, "midiCtrlGraphMergeEraseWysiwyg", MusEGlobal::config.midiCtrlGraphMergeEraseWysiwyg);
      xml.intTag(level, "lv2UiBehavior", static_cast<int>(MusEGlobal::config.lv2UiBehavior));
      xml.strTag(level, "mixdownPath", MusEGlobal::config.mixdownPath);
      xml.intTag(level, "showNoteNamesInPianoRoll", MusEGlobal::config.showNoteNamesInPianoRoll);
      xml.intTag(level, "showNoteTooltips", MusEGlobal::config.showNoteTooltips);
      xml.intTag(level, "noPluginScaling", MusEGlobal::config.noPluginScaling);
      xml.intTag(level, "openMDIWinMaximized", MusEGlobal::config.openMDIWinMaximized);
      xml.intTag(level, "keepTransportWindowOnTop", MusEGlobal::config.keepTransportWindowOnTop);

      for (int i = 1; i < NUM_FONTS; ++i) {
//          for (int i = 0; i < NUM_FONTS; ++i) {
            xml.strTag(level, QString("font") + QString::number(i), MusEGlobal::config.fonts[i].toString());
            }
      xml.intTag(level, "autoAdjustFontSize", MusEGlobal::config.autoAdjustFontSize);
            
      xml.intTag(level, "globalAlphaBlend", MusEGlobal::config.globalAlphaBlend);
      
      MusECore::writeConfigurationColors(level, xml);
      
      xml.intTag(level, "mtctype", MusEGlobal::mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        MusEGlobal::mtcOffset.h(), MusEGlobal::mtcOffset.m(), MusEGlobal::mtcOffset.s(),
        MusEGlobal::mtcOffset.f(), MusEGlobal::mtcOffset.sf());
      xml.intTag(level, "extSync", MusEGlobal::extSyncFlag);
      xml.intTag(level, "useJackTransport", MusEGlobal::config.useJackTransport);
      xml.intTag(level, "timebaseMaster", MusEGlobal::config.timebaseMaster);
      
      xml.qrectTag(level, "geometryMain",      MusEGlobal::config.geometryMain);
      xml.qrectTag(level, "geometryTransport", MusEGlobal::config.geometryTransport);
      xml.qrectTag(level, "geometryBigTime",   MusEGlobal::config.geometryBigTime);

      xml.intTag(level, "bigtimeVisible", MusEGlobal::config.bigTimeVisible);
      xml.intTag(level, "transportVisible", MusEGlobal::config.transportVisible);
      
      xml.intTag(level, "mixer1Visible", MusEGlobal::config.mixer1Visible);
      xml.intTag(level, "mixer2Visible", MusEGlobal::config.mixer2Visible);
      // True = Write global config.
      MusEGlobal::config.mixer1.write(level, xml, true);
      MusEGlobal::config.mixer2.write(level, xml, true);

      xml.intTag(level, "showSplashScreen", MusEGlobal::config.showSplashScreen);
      xml.intTag(level, "canvasShowPartType", MusEGlobal::config.canvasShowPartType);
      xml.intTag(level, "canvasShowPartEvent", MusEGlobal::config.canvasShowPartEvent);
      xml.intTag(level, "canvasShowGrid", MusEGlobal::config.canvasShowGrid);
      xml.strTag(level, "canvasBgPixmap", MusEGlobal::config.canvasBgPixmap);
      xml.strTag(level, "canvasCustomBgList", MusEGlobal::config.canvasCustomBgList.join(";"));

      xml.intTag(level, "maxAliasedPointSize", MusEGlobal::config.maxAliasedPointSize);

      xml.intTag(level, "iconSize", MusEGlobal::config.iconSize);
      xml.intTag(level, "cursorSize", MusEGlobal::config.cursorSize);
      xml.intTag(level, "cascadeStylesheets", MusEGlobal::config.cascadeStylesheets);
      xml.intTag(level, "showIconsInMenus", MusEGlobal::config.showIconsInMenus);
      
      MusEGlobal::writePluginGroupConfiguration(level, xml);

      writeSeqConfiguration(level, xml, false);

      MusEGui::DrumEdit::writeConfiguration(level, xml);
      MusEGui::PianoRoll::writeConfiguration(level, xml);
      MusEGui::ScoreEdit::write_configuration(level, xml);
      MusEGui::MasterEdit::writeConfiguration(level, xml);
      MusEGui::WaveEdit::writeConfiguration(level, xml);
      MusEGui::ListEdit::writeConfiguration(level, xml);
      MusEGui::ClipListEdit::writeConfiguration(level, xml);
      MusEGui::LMaster::writeConfiguration(level, xml);
      MusEGui::MarkerView::writeConfiguration(level, xml);
      arrangerView->writeConfiguration(level, xml);
      
      MusEGui::write_function_dialog_config(level, xml);

      MusEGui::writeShortCuts(level, xml);
      xml.etag(level, "configuration");
      }

//---------------------------------------------------------
//   writeConfiguration
//    write song specific configuration
//---------------------------------------------------------

void MusE::writeConfiguration(int level, MusECore::Xml& xml) const
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

      xml.intTag(level, "mtctype", MusEGlobal::mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        MusEGlobal::mtcOffset.h(), MusEGlobal::mtcOffset.m(), MusEGlobal::mtcOffset.s(),
        MusEGlobal::mtcOffset.f(), MusEGlobal::mtcOffset.sf());
      xml.uintTag(level, "sendClockDelay", MusEGlobal::syncSendFirstClockDelay);
      xml.intTag(level, "useJackTransport", MusEGlobal::config.useJackTransport);
      xml.intTag(level, "timebaseMaster", MusEGlobal::config.timebaseMaster);
      xml.intTag(level, "syncRecFilterPreset", MusEGlobal::syncRecFilterPreset);
      xml.doubleTag(level, "syncRecTempoValQuant", MusEGlobal::syncRecTempoValQuant);
      xml.intTag(level, "extSync", MusEGlobal::extSyncFlag);
      
      xml.intTag(level, "bigtimeVisible",   viewBigtimeAction->isChecked());
      xml.intTag(level, "transportVisible", viewTransportAction->isChecked());
      
      xml.geometryTag(level, "geometryMain", this); // FINDME: maybe remove this? do we want
                                                    // the main win to jump around when loading?
      if (transport)
            xml.geometryTag(level, "geometryTransport", transport);
      if (bigtime)
            xml.geometryTag(level, "geometryBigTime", bigtime);
      
      // i thought this was obsolete, but it seems to be necessary (flo)
      xml.intTag(level, "arrangerVisible", viewArrangerAction->isChecked());
      xml.intTag(level, "markerVisible", viewMarkerAction->isChecked());
      // but storing the geometry IS obsolete. this is really
      // done by TopLevel::writeConfiguration

      xml.intTag(level, "mixer1Visible",    viewMixerAAction->isChecked());
      xml.intTag(level, "mixer2Visible",    viewMixerBAction->isChecked());
      // False = Write song-specific config.
      MusEGlobal::config.mixer1.write(level, xml, false);
      MusEGlobal::config.mixer2.write(level, xml, false);

      writeSeqConfiguration(level, xml, true);

      MusEGui::write_function_dialog_config(level, xml);

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
        // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
        midiSyncConfig = new MusEGui::MidiSyncConfig;
        
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
            // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
            midiFileConfig = new MusEGui::MidiFileConfig();
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
          // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
          globalSettingsConfig = new MusEGui::GlobalSettingsConfig();

      if (globalSettingsConfig->isVisible()) {
          globalSettingsConfig->raise();
          globalSettingsConfig->activateWindow();
          }
      else
          globalSettingsConfig->show();
      }


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
      importDefaultInstr->clear();
      for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
        if(!(*i)->isSynti())                            // Sorry, no synths for now.
          importDefaultInstr->addItem((*i)->iname());
      int idx = importDefaultInstr->findText(MusEGlobal::config.importMidiDefaultInstr);
      if(idx != -1)
        importDefaultInstr->setCurrentIndex(idx);
      
      QString defInstr = importDefaultInstr->currentText();
      if(!defInstr.isEmpty())
        MusEGlobal::config.importMidiDefaultInstr = defInstr;
      
      int divisionIdx = 2;
      switch(MusEGlobal::config.midiDivision) {
            case 96:  divisionIdx = 0; break;
            case 192:  divisionIdx = 1; break;
            case 384:  divisionIdx = 2; break;
            }
      divisionCombo->setCurrentIndex(divisionIdx);
      formatCombo->setCurrentIndex(MusEGlobal::config.smfFormat);
      extendedFormat->setChecked(MusEGlobal::config.extendedMidi);
      copyrightEdit->setText(MusEGlobal::config.copyright);
      runningStatus->setChecked(MusEGlobal::config.expRunningStatus);
      optNoteOffs->setChecked(MusEGlobal::config.expOptimNoteOffs);
      twoByteTimeSigs->setChecked(MusEGlobal::config.exp2ByteTimeSigs);
      splitPartsCheckBox->setChecked(MusEGlobal::config.importMidiSplitParts);
// Obsolete. There is only 'New' drum tracks now.
//       newDrumsCheckbox->setChecked(MusEGlobal::config.importMidiNewStyleDrum);
//       oldDrumsCheckbox->setChecked(!MusEGlobal::config.importMidiNewStyleDrum);
      importDevNameMetas->setChecked(MusEGlobal::config.importDevNameMetas);
      importInstrNameMetas->setChecked(MusEGlobal::config.importInstrNameMetas);
      exportPortDeviceSMF0->setChecked(MusEGlobal::config.exportPortDeviceSMF0);
      drumMapOverrides->setChecked(MusEGlobal::config.exportDrumMapOverrides);
      channelOverridesToNewTrack->setChecked(MusEGlobal::config.exportChannelOverridesToNewTrack);
      exportPortMetas->setChecked(MusEGlobal::config.exportPortsDevices & MusEGlobal::PORT_NUM_META);
      exportDeviceNameMetas->setChecked(MusEGlobal::config.exportPortsDevices & MusEGlobal::DEVICE_NAME_META);
      exportModeSysexes->setChecked(MusEGlobal::config.exportModeInstr & MusEGlobal::MODE_SYSEX);
      exportInstrumentNames->setChecked(MusEGlobal::config.exportModeInstr & MusEGlobal::INSTRUMENT_NAME_META);
          
      }

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
      {
      QString defInstr = importDefaultInstr->currentText();
      if(!defInstr.isEmpty())
        MusEGlobal::config.importMidiDefaultInstr = defInstr;
      
      int divisionIdx = divisionCombo->currentIndex();

      int divisions[3] = { 96, 192, 384 };
      if (divisionIdx >= 0 && divisionIdx < 3)
            MusEGlobal::config.midiDivision = divisions[divisionIdx];
      MusEGlobal::config.extendedMidi = extendedFormat->isChecked();
      MusEGlobal::config.smfFormat    = formatCombo->currentIndex();
      MusEGlobal::config.copyright    = copyrightEdit->text();
      MusEGlobal::config.expRunningStatus = runningStatus->isChecked();
      MusEGlobal::config.expOptimNoteOffs = optNoteOffs->isChecked();
      MusEGlobal::config.exp2ByteTimeSigs = twoByteTimeSigs->isChecked();
      MusEGlobal::config.importMidiSplitParts = splitPartsCheckBox->isChecked();
// Obsolete. There is only 'New' drum tracks now.
//       MusEGlobal::config.importMidiNewStyleDrum = newDrumsCheckbox->isChecked();
      
      MusEGlobal::config.importDevNameMetas = importDevNameMetas->isChecked();
      MusEGlobal::config.importInstrNameMetas = importInstrNameMetas->isChecked();
      MusEGlobal::config.exportPortDeviceSMF0 = exportPortDeviceSMF0->isChecked();  
      MusEGlobal::config.exportDrumMapOverrides = drumMapOverrides->isChecked();
      MusEGlobal::config.exportChannelOverridesToNewTrack = channelOverridesToNewTrack->isChecked();

      MusEGlobal::config.exportPortsDevices = 0;
      if(exportPortMetas->isChecked())
        MusEGlobal::config.exportPortsDevices |= MusEGlobal::PORT_NUM_META;
      if(exportDeviceNameMetas->isChecked())
        MusEGlobal::config.exportPortsDevices |= MusEGlobal::DEVICE_NAME_META;

      MusEGlobal::config.exportModeInstr = 0;
      if(exportModeSysexes->isChecked())
        MusEGlobal::config.exportModeInstr |= MusEGlobal::MODE_SYSEX;
      if(exportInstrumentNames->isChecked())
        MusEGlobal::config.exportModeInstr |= MusEGlobal::INSTRUMENT_NAME_META;
      
      // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
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

} // namespace MusEGui


namespace MusEGlobal {

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StripConfig::write(int level, MusECore::Xml& xml) const
      {
      if(_serial < 0)
        return;
      // Do NOT save if there is no corresponding track.
      const MusECore::TrackList* tl = song->tracks();
      const int idx = tl->indexOfSerial(_serial);
      if(idx < 0)
        return;
      xml.nput(level, "<StripConfig trackIdx=\"%d\"", idx);

      xml.nput(level, " visible=\"%d\"", _visible);
      if(_width >= 0)
        xml.nput(level, " width=\"%d\"", _width);
      xml.put(" />");
      
      //xml.put(">");
      //level++;
      // TODO: Anything else to add? ...
      //xml.etag(level, "StripConfig");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StripConfig::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                          xml.unknown("StripConfig");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "trackIdx") {
                              _tmpFileIdx = xml.s2().toInt();
                              }
                        else if (tag == "visible") {
                              _visible = xml.s2().toInt();
                              }
                        else if (tag == "width") {
                              _width = xml.s2().toInt();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "StripConfig")
                            return;
                  default:
                        break;
                  }
            }
      
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MixerConfig::write(int level, MusECore::Xml& xml, bool global) const
      {
      xml.tag(level++, "Mixer");

      xml.strTag(level, "name", name);
      
      xml.qrectTag(level, "geometry", geometry);
      
      xml.intTag(level, "showMidiTracks",   showMidiTracks);
      xml.intTag(level, "showDrumTracks",   showDrumTracks);
      xml.intTag(level, "showNewDrumTracks",   showNewDrumTracks);
      xml.intTag(level, "showInputTracks",  showInputTracks);
      xml.intTag(level, "showOutputTracks", showOutputTracks);
      xml.intTag(level, "showWaveTracks",   showWaveTracks);
      xml.intTag(level, "showGroupTracks",  showGroupTracks);
      xml.intTag(level, "showAuxTracks",    showAuxTracks);
      xml.intTag(level, "showSyntiTracks",  showSyntiTracks);

      xml.intTag(level, "displayOrder", displayOrder);

      // Specific to song file.
      if(!global)
      {
        if(!stripConfigList.empty())
        {
          const int sz = stripConfigList.size();
          for(int i = 0; i < sz; ++i)
            stripConfigList.at(i).write(level, xml);
        }
      }

      xml.etag(level, "Mixer");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MixerConfig::read(MusECore::Xml& xml)
      {
      bool ignore_next_visible = false;
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "name")
                              name = xml.parse1();
                        else if (tag == "geometry")
                              geometry = readGeometry(xml, tag);
                        else if (tag == "showMidiTracks")
                              showMidiTracks = xml.parseInt();
                        else if (tag == "showDrumTracks")
                              showDrumTracks = xml.parseInt();
                        else if (tag == "showNewDrumTracks")
                              showNewDrumTracks = xml.parseInt();
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
                        else if (tag == "displayOrder")
                              displayOrder = (DisplayOrder)xml.parseInt();
                        // Obsolete. Support old songs.
                        else if (tag == "StripName")
                        {
                          const QString s = xml.parse1();
                          // Protection from duplicates in song file, observed (old flaws?).
                          if(stripOrder.contains(s))
                            ignore_next_visible = true;
                          else
                            stripOrder.append(s);
                        }
                        // Obsolete. Support old songs.
                        else if (tag == "StripVisible")
                        {
                          // Protection from duplicates in song file, observed (old flaws?).
                          if(ignore_next_visible)
                          {
                            xml.parseInt();
                            ignore_next_visible = false;
                          }
                          else
                          {
                            stripVisibility.append(xml.parseInt() == 0 ? false : true );
                          }
                        }
                        else if (tag == "StripConfig") {
                              StripConfig sc;
                              sc.read(xml);
                              if(sc._tmpFileIdx >= 0)
                                stripConfigList.append(sc);
                        }
                        else
                              xml.unknown("Mixer");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Mixer")
                            return;
                  default:
                        break;
                  }
            }
      
      }

} // namespace MusEGlobal

