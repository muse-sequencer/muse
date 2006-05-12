//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "al/al.h"
#include "muse.h"
#include "shortcuts.h"
#include "transport.h"
#include "widgets/bigtime.h"
#include "conf.h"
#include "gconfig.h"
#include "al/xml.h"
#include "widgets/midisync.h"
// #include "genset.h"
#include "sync.h"
#include "mixer/mixer.h"
#include "globals.h"
#include "midirc.h"
#include "awl/tcanvas.h"
#include "midiedit/pianoroll.h"
#include "midiedit/drumedit.h"

extern void writeMidiTransforms(Xml& xml);
extern void readMidiTransform(QDomNode);

//---------------------------------------------------------
//   readColor
//---------------------------------------------------------

QColor readColor(QDomNode node)
      {
      QDomElement e = node.toElement();
      int r = e.attribute("r","0").toInt();
      int g = e.attribute("g","0").toInt();
      int b = e.attribute("b","0").toInt();
      return QColor(r, g, b);
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void readConfiguration(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s(e.text());
            int i = s.toInt();
            if (tag == "theme")
                  config.style = s;
            else if (tag == "font0")
                  config.fonts[0]->fromString(s);
            else if (tag == "font1")
                  config.fonts[1]->fromString(s);
            else if (tag == "font2")
                  config.fonts[2]->fromString(s);
            else if (tag == "font3")
                  config.fonts[3]->fromString(s);
            else if (tag == "font4")
                  config.fonts[4]->fromString(s);
            else if (tag == "font5")
                  config.fonts[5]->fromString(s);
            else if (tag == "palette0")
                  config.palette[0] = readColor(node);
            else if (tag == "palette1")
                  config.palette[1] = readColor(node);
            else if (tag == "palette2")
                  config.palette[2] = readColor(node);
            else if (tag == "palette3")
                  config.palette[3] = readColor(node);
            else if (tag == "palette4")
                  config.palette[4] = readColor(node);
            else if (tag == "palette5")
                  config.palette[5] = readColor(node);
            else if (tag == "palette6")
                  config.palette[6] = readColor(node);
            else if (tag == "palette7")
                  config.palette[7] = readColor(node);
            else if (tag == "palette8")
                  config.palette[8] = readColor(node);
            else if (tag == "palette9")
                  config.palette[9] = readColor(node);
            else if (tag == "palette10")
                  config.palette[10] = readColor(node);
            else if (tag == "palette11")
                  config.palette[11] = readColor(node);
            else if (tag == "palette12")
                  config.palette[12] = readColor(node);
            else if (tag == "palette13")
                  config.palette[13] = readColor(node);
            else if (tag == "palette14")
                  config.palette[14] = readColor(node);
            else if (tag == "palette15")
                  config.palette[15] = readColor(node);
            else if (tag == "palette16")
                  config.palette[16] = readColor(node);
            else if (tag == "selectPartBg")
                  config.selectPartBg = readColor(node);
            else if (tag == "outputTrackBg")
                  config.trackBg[Track::AUDIO_OUTPUT] = readColor(node);
            else if (tag == "groupTrackBg")
                  config.trackBg[Track::AUDIO_GROUP] = readColor(node);
            else if (tag == "auxTrackBg")
                  config.trackBg[Track::AUDIO_AUX] = readColor(node);
            else if (tag == "waveTrackBg")
                  config.trackBg[Track::WAVE] = readColor(node);
            else if (tag == "inputTrackBg")
                  config.trackBg[Track::AUDIO_INPUT] = readColor(node);
            else if (tag == "synthTrackBg")
                  config.trackBg[Track::AUDIO_SOFTSYNTH] = readColor(node);
            else if (tag == "midiTrackBg")
                  config.trackBg[Track::MIDI] = readColor(node);
            else if (tag == "midiOutputBg")
                  config.trackBg[Track::MIDI_OUT] = readColor(node);
            else if (tag == "midiInputBg")
                  config.trackBg[Track::MIDI_IN] = readColor(node);
            else if (tag == "midiChannelBg")
                  config.trackBg[Track::MIDI_CHANNEL] = readColor(node);
            else if (tag == "midiSyntiBg")
                  config.trackBg[Track::MIDI_SYNTI] = readColor(node);
            else if (tag == "extendedMidi")
                  config.extendedMidi = i;
            else if (tag == "midiExportDivision")
                  config.midiDivision = i;
            else if (tag == "copyright")
                  config.copyright = s;
            else if (tag == "smfFormat")
                  config.smfFormat = i;
            else if (tag == "bigtimeVisible")
                  config.bigTimeVisible = i;
            else if (tag == "transportVisible")
                  config.transportVisible = i;
            else if (tag == "mixer1Visible")
                  config.mixer1Visible = i;
            else if (tag == "mixer2Visible")
                  config.mixer2Visible = i;
            else if (tag == "showSplashScreen")
                  config.showSplashScreen = i;
            else if (tag == "canvasShowPartType")
                  config.canvasShowPartType = i;
            else if (tag == "canvasShowPartEvent")
                  config.canvasShowPartEvent = i;
            else if (tag == "canvasShowGrid")
                  config.canvasShowGrid = i;
            else if (tag == "canvasBgPixmap")
                  config.canvasBgPixmap = s;
            else if (tag == "canvasUsePixmap")
                  config.canvasUseBgPixmap = i;
            else if (tag == "geometryMain")
                  config.geometryMain = AL::readGeometry(node);
            else if (tag == "geometryTransport")
                  config.geometryTransport = AL::readGeometry(node);
            else if (tag == "geometryBigTime")
                  config.geometryBigTime = AL::readGeometry(node);
            else if (tag == "geometryPianoroll")
                  config.geometryPianoroll = AL::readGeometry(node);
            else if (tag == "geometryDrumedit")
                  config.geometryDrumedit = AL::readGeometry(node);
            else if (tag == "mixer1")
                  config.mixer1.read(node);
            else if (tag == "mixer2")
                  config.mixer2.read(node);
            else if (tag == "bigtimeForegroundcolor")
                  config.bigTimeForegroundColor = readColor(node);
            else if (tag == "bigtimeBackgroundcolor")
                  config.bigTimeBackgroundColor = readColor(node);
//            else if (tag == "transportHandleColor")
//                  config.transportHandleColor = readColor(node);
            else if (tag == "freewheelMode")
                  config.useJackFreewheelMode = i;
            else if (tag == "txDeviceId")
                  txDeviceId = i;
            else if (tag == "rxDeviceId")
                  rxDeviceId = i;
            else if (tag == "mtctype")
                  AL::mtcType = i;
            else if (tag == "extSync")
                  extSyncFlag = i;
            else if (tag == "syncgentype") {
                  // for compatibility
                  int syncGenType= i;
                  genMTCSync = syncGenType == 1;
                  genMCSync = syncGenType == 2;
                  }
            else if (tag == "genMTCSync")
                  genMTCSync = i;
            else if (tag == "genMCSync")
                  genMCSync = i;
            else if (tag == "genMMC")
                  genMMC = i;
            else if (tag == "acceptMTC")
                  acceptMTC = i;
            else if (tag == "acceptMMC")
                  acceptMMC = i;
            else if (tag == "acceptMC")
                  acceptMC = i;
            else if (tag == "mtcoffset") {
                  QStringList l = s.simplified().split(":", QString::SkipEmptyParts);
                  if (l.size() != 5) {
                        printf("cannot convert mtcoffset <%s>n\n", s.toLatin1().data());
                        }
                  else {
	                  int h = l.at(0).toInt();
      	            int m = l.at(0).toInt();
            	      int s = l.at(0).toInt();
                  	int f = l.at(0).toInt();
	                  int sf = l.at(0).toInt();
      	            mtcOffset = MTC(h, m, s, f, sf);
                        }
                  }
            else if (tag == "shortcuts")
                  readShortCuts(node.firstChild());
            else if (tag == "midiRC")
                  midiRCList.read(node);
            else if (tag == "division")
                  config.division = i;
            else if (tag == "guiDivision")
                  config.guiDivision = i;
            else if (tag == "rtcTicks")
                  config.rtcTicks = i;
            else if (tag == "minMeter")
                  config.minMeter = i;
            else if (tag == "minSlider")
                  config.minSlider = s.toFloat();
            else if (tag == "guiRefresh")
                  config.guiRefresh = i;
            else if (tag == "peakHoldTime")
                  config.peakHoldTime = i;
            else if (tag == "helpBrowser")
                  config.helpBrowser = s;
//TD            else if (tag == "midiTransform")
//                  readMidiTransform(node.firstChild());
            else if (tag == "startMode")
                  config.startMode = (StartMode)i;
            else if (tag == "startProject")
                  config.startProject = s;
            else if (tag == "followMode")
                  TimeCanvas::followMode = (FollowMode)i;
            else if (tag == "defaultMidiInputDevice")
                  config.defaultMidiInputDevice = s;
            else if (tag == "defaultMidiOutputDevice")
                  config.defaultMidiOutputDevice = s;
            else if (tag == "defaultMidiInstrument")
                  config.defaultMidiInstrument = s;
            else if (tag == "connectToAllMidiDevices")
                  config.connectToAllMidiDevices = i;
            else if (tag == "connectToAllMidiTracks")
                  config.connectToAllMidiTracks = i;
            else if (tag == "createDefaultMidiInput")
                  config.createDefaultMidiInput = i;
            else if (tag == "projectPath")
                  config.projectPath = s;
            else if (tag == "templatePath")
                  config.templatePath = s;
            else if (tag == "PianoRoll")
                  PianoRoll::readConfiguration(node);
            else if (tag == "DrumEdit")
                  DrumEdit::readConfiguration(node);
            else {
                  printf("MusE:readConfiguration(): unknown tag %s\n",
                     e.tagName().toLatin1().data());
                  }
            }
      }

//---------------------------------------------------------
//   probeMachineSpecificConfiguration
//---------------------------------------------------------

static void probeMachineSpecificConfiguration()
      {
      // set a default help browser (crude way to find out)
      if (!system("which konqueror &>/dev/null")) {
           config.helpBrowser = QString("konqueror");
           }
      else if (!system("which opera &>/dev/null")) {
           config.helpBrowser = QString("opera");
           }
      else if (!system("which firebird &>/dev/null")) {
           config.helpBrowser = QString("firebird");
           }
      else if (!system("which mozilla &>/dev/null")) {
           config.helpBrowser = QString("mozilla");
           }
      else {
             // was not able to find a browser
           }
      // More preconfiguration
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

bool readConfiguration()
      {
      if (debugMsg)
            printf("readConfiguration <%s>\n", configName.toLatin1().data());

      QFile qf(configName);
      if (!qf.open(QIODevice::ReadOnly)) {
            if (debugMsg || debugMode)
                  fprintf(stderr, "NO Config File <%s> found\n", configName.toLatin1().data());

            // if the config file does not exist launch probeMachineSpecificConfiguration
            probeMachineSpecificConfiguration();
            return true;
            }
      if (debugMsg)
            printf("readConfiguration <%s>\n", configName.toLatin1().data());

      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(&qf, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error reading med file: %s\n", error.toLatin1().data());
            return true;
            }
      QDomNode node = doc.documentElement();
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "muse") {
                  node = node.firstChild();
                  // QString version = e.attribute(QString("version"));
                  while (!node.isNull()) {
                        QDomElement e = node.toElement();
                        if (e.tagName() == "configuration")
                              readConfiguration(node.firstChild());
                        else
                              printf("MusE:readConfiguration(): unknown tag %s\n", e.tagName().toLatin1().data());
                        node = node.nextSibling();
                        }
                  }
            else
                  printf("MusE:readConfiguration() %s not supported\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      return false;
      }

//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void MusE::writeGlobalConfiguration() const
      {
      QFile f(configName);
      if (!f.open(QIODevice::WriteOnly)) {
            printf("save configuration to <%s> failed: %s\n",
               configName.toLatin1().data(), strerror(errno));
            return;
            }
      Xml xml(&f);
      xml.header();
      xml.tag("muse version=\"2.0\"");
      writeGlobalConfiguration(xml);
      xml.etag("muse");
      f.close();
      }

void MusE::writeGlobalConfiguration(Xml& xml) const
      {
      xml.tag("configuration");

      xml.intTag("division", config.division);
      xml.intTag("rtcTicks", config.rtcTicks);
      xml.intTag("minMeter", config.minMeter);
      xml.doubleTag("minSlider", config.minSlider);
      xml.intTag("guiRefresh", config.guiRefresh);
      xml.intTag("peakHoldTime", config.peakHoldTime);
      xml.strTag("helpBrowser", config.helpBrowser);
      xml.intTag("extendedMidi", config.extendedMidi);
      xml.intTag("midiExportDivision", config.midiDivision);
      xml.intTag("guiDivision", config.guiDivision);
      xml.strTag("copyright", config.copyright);
      xml.intTag("smfFormat", config.smfFormat);
      xml.intTag("startMode", config.startMode);
      xml.strTag("startProject", config.startProject);
      xml.intTag("freewheelMode", config.useJackFreewheelMode);

      xml.intTag("txDeviceId", txDeviceId);
      xml.intTag("rxDeviceId", rxDeviceId);
      xml.strTag("theme", config.style);

      for (int i = 0; i < 6; ++i) {
            char buffer[32];
            sprintf(buffer, "font%d", i);
            xml.strTag(buffer, config.fonts[i]->toString());
            }
      for (int i = 0; i < 16; ++i) {
            char buffer[32];
            sprintf(buffer, "palette%d", i);
            xml.colorTag(buffer, config.palette[i]);
            }

      xml.colorTag("selectPartBg",  config.selectPartBg);

      static const char* colorNames[Track::TRACK_TYPES] = {
            "outputTrackBg",
            "groupTrackBg",
            "auxTrackBg",
            "waveTrackBg",
            "inputTrackBg",
            "synthTrackBg",
            "midiTrackBg",
            "midiOutputBg",
            "midiInputBg",
            "midiChannelBg",
            "midiSyntiBg"
            };
      for (int i = 0; i < Track::TRACK_TYPES; ++i)
            xml.colorTag(colorNames[i],  config.trackBg[i]);

      xml.intTag("mtctype", AL::mtcType);

      xml.tag("mtcoffset");
      xml.put("%02d:%02d:%02d:%02d:%02d",
        mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
        mtcOffset.f(), mtcOffset.sf());
      xml.etag("mtcoffset");

      xml.intTag("extSync", extSyncFlag);
      xml.intTag("genMTCSync", genMTCSync);
      xml.intTag("genMCSync", genMCSync);
      xml.intTag("genMMC", genMMC);
      xml.intTag("acceptMTC", acceptMTC);
      xml.intTag("acceptMMC", acceptMMC);
      xml.intTag("acceptMC", acceptMC);

      xml.qrectTag("geometryMain",      config.geometryMain);
      xml.qrectTag("geometryTransport", config.geometryTransport);
      xml.qrectTag("geometryBigTime",   config.geometryBigTime);
      xml.qrectTag("geometryPianoroll", config.geometryPianoroll);
      xml.qrectTag("geometryDrumedit",  config.geometryDrumedit);

      xml.intTag("bigtimeVisible", config.bigTimeVisible);
      xml.intTag("transportVisible", config.transportVisible);

      xml.intTag("mixer1Visible", config.mixer1Visible);
      xml.intTag("mixer2Visible", config.mixer2Visible);

      config.mixer1.write(xml, "mixer1");
      config.mixer2.write(xml, "mixer2");

      xml.intTag("showSplashScreen", config.showSplashScreen);
      xml.intTag("canvasShowPartType", config.canvasShowPartType);
      xml.intTag("canvasShowPartEvent", config.canvasShowPartEvent);
      xml.intTag("canvasShowGrid", config.canvasShowGrid);
      xml.intTag("canvasUsePixmap", config.canvasUseBgPixmap);
      xml.strTag("canvasBgPixmap", config.canvasBgPixmap);

//      xml.colorTag("transportHandleColor",  config.transportHandleColor);
      xml.colorTag("bigtimeForegroundcolor", config.bigTimeForegroundColor);
      xml.colorTag("bigtimeBackgroundcolor", config.bigTimeBackgroundColor);

      writeShortCuts(xml);
      midiRCList.write(xml);
      xml.intTag("followMode", TimeCanvas::followMode);

      xml.strTag("defaultMidiInputDevice", config.defaultMidiInputDevice);
      xml.strTag("defaultMidiOutputDevice", config.defaultMidiOutputDevice);
      xml.strTag("defaultMidiInstrument", config.defaultMidiInstrument);
      xml.intTag("connectToAllMidiDevices", config.connectToAllMidiDevices);
      xml.intTag("connectToAllMidiTracks", config.connectToAllMidiTracks);
      xml.intTag("createDefaultMidiInput", config.createDefaultMidiInput);
      xml.strTag("projectPath", config.projectPath);
      xml.strTag("templatePath", config.templatePath);

      PianoRoll::writeConfiguration(xml);
      DrumEdit::writeConfiguration(xml);
      xml.etag("configuration");
      }

//---------------------------------------------------------
//   writeConfiguration
//    write song specific configuration
//---------------------------------------------------------

void MusE::writeConfiguration(Xml& xml) const
      {
      xml.tag("configuration");

      xml.intTag("txDeviceId", txDeviceId);
      xml.intTag("rxDeviceId", rxDeviceId);

      xml.intTag("mtctype", AL::mtcType);
      xml.tag("mtcoffset");
      xml.put("%02d:%02d:%02d:%02d:%02d",
        mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
        mtcOffset.f(), mtcOffset.sf());
      xml.etag("mtcoffset");
      xml.intTag("extSync", extSyncFlag);
      xml.intTag("genMTCSync", genMTCSync);
      xml.intTag("genMCSync", genMCSync);
      xml.intTag("genMMC", genMMC);
      xml.intTag("acceptMTC", acceptMTC);
      xml.intTag("acceptMMC", acceptMMC);
      xml.intTag("acceptMC", acceptMC);

      xml.intTag("bigtimeVisible",   bt_id->isChecked());
      xml.intTag("transportVisible", tr_id->isChecked());

      xml.geometryTag("geometryMain", this);
      if (transport)
            xml.geometryTag("geometryTransport", transport);
      if (bigtime)
            xml.geometryTag("geometryBigTime", bigtime);

      xml.intTag("mixer1Visible",    aid1a->isChecked());
      xml.intTag("mixer2Visible",    aid1b->isChecked());
      if (mixer1)
            mixer1->write(xml, "mixer1");
      if (mixer2)
            mixer2->write(xml, "mixer2");

//TD      writeMidiTransforms(xml);
      xml.etag("configuration");
      }

//---------------------------------------------------------
//   configMidiSync
//---------------------------------------------------------

void MusE::configMidiSync()
      {
      if (!midiSyncConfig) {
            midiSyncConfig = new MidiSyncConfig(this);
            connect(midiSyncConfig, SIGNAL(syncChanged()), SLOT(syncChanged()));
            }
      midiSyncConfig->raise();
      midiSyncConfig->show();
      }

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void MusE::syncChanged()
      {
      transport->syncChanged();
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
//TD          midiFileConfig->setActiveWindow();
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
      switch(config.midiDivision) {
            case 96:  divisionIdx = 0; break;
            case 192:  divisionIdx = 1; break;
            case 384:  divisionIdx = 2; break;
            }
      divisionCombo->setCurrentIndex(divisionIdx);
      formatCombo->setCurrentIndex(config.smfFormat);
      extendedFormat->setChecked(config.extendedMidi);
      copyrightEdit->setText(config.copyright);
      }

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
      {
      int divisionIdx = divisionCombo->currentIndex();

      int divisions[3] = { 96, 192, 384 };
      if (divisionIdx >= 0 && divisionIdx < 3)
            config.midiDivision = divisions[divisionIdx];
      config.extendedMidi = extendedFormat->isChecked();
      config.smfFormat    = formatCombo->currentIndex();
      config.copyright    = copyrightEdit->text();

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

//void MusE::configGlobalSettings()
//      {
//      if (!globalSettingsConfig)
//            globalSettingsConfig = new GlobalSettingsConfig();
//
//      if (globalSettingsConfig->isVisible()) {
//          globalSettingsConfig->raise();
////TD          globalSettingsConfig->setActiveWindow();
//          }
//      else
//          globalSettingsConfig->show();
//      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MixerConfig::write(Xml& xml, const char* name)
      {
      xml.tag("%s", name);
      xml.qrectTag("geometry",       geometry);
      xml.intTag("showMidiTracks",   showMidiTracks);
      xml.intTag("showMidiSyntiPorts", showMidiSyntiPorts);
      xml.intTag("showMidiTracks",   showMidiTracks);
      xml.intTag("showOutputTracks", showOutputTracks);
      xml.intTag("showWaveTracks",   showWaveTracks);
      xml.intTag("showGroupTracks",  showGroupTracks);
      xml.intTag("showInputTracks",  showInputTracks);
      xml.intTag("showAuxTracks",    showAuxTracks);
      xml.intTag("showSyntiTracks",  showSyntiTracks);
      xml.intTag("showMidiInPorts",  showMidiInPorts);
      xml.intTag("showMidiOutPorts", showMidiOutPorts);
      xml.intTag("showMidiChannels", showMidiChannels);
      xml.etag("%s", name);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MixerConfig::read(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s = e.text();
            int i = s.toInt();
            if (tag == "geometry")
                  geometry = AL::readGeometry(node);
            else if (tag == "showMidiTracks")
                  showMidiTracks = i;
            else if (tag == "showMidiSyntiPorts")
                  showMidiSyntiPorts = i;
            else if (tag == "showOutputTracks")
                  showOutputTracks = i;
            else if (tag == "showWaveTracks")
                  showWaveTracks = i;
            else if (tag == "showGroupTracks")
                  showGroupTracks = i;
            else if (tag == "showInputTracks")
                  showInputTracks = i;
            else if (tag == "showAuxTracks")
                  showAuxTracks = i;
            else if (tag == "showSyntiTracks")
                  showSyntiTracks = i;
            else if (tag == "showMidiInPorts")
                  showMidiInPorts = i;
            else if (tag == "showMidiOutPorts")
                  showMidiOutPorts = i;
            else if (tag == "showMidiChannels")
                  showMidiChannels = i;
            else
                  printf("MusE:MixerConfig: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

