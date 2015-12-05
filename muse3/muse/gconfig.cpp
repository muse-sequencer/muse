//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.cpp,v 1.15.2.13 2009/12/01 03:52:40 terminator356 Exp $
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

#include "gconfig.h"

namespace MusEGlobal {

GlobalConfigValues config = {
      170,                        // globalAlphaBlend    
      {
        QColor(0xff, 0xff, 0xff),   // palette
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff),
        QColor(0xff, 0xff, 0xff)
        },
      {
        QColor(255,  232,  140),   // part colors
        QColor(0xff, 0x00, 0x00),
        QColor(0x00, 0xff, 0x00),
        QColor(0x00, 0x00, 0xff),
        QColor(0xff, 0xff, 0x00),
        QColor(0x00, 0xff, 0xff),
        QColor(0xff, 0x00, 0xff),
        QColor(0x9f, 0xc7, 0xef),
        QColor(0x00, 0xff, 0x7f),
        QColor(0x7f, 0x00, 0x00),
        QColor(0x00, 0x7f, 0x00),
        QColor(0x00, 0x00, 0x7f),
        QColor(0x7f, 0x7f, 0x3f),
        QColor(0x00, 0x7f, 0x7f),
        QColor(0x7f, 0x00, 0x7f),
        QColor(0x00, 0x7f, 0xff),
        QColor(0x00, 0x3f, 0x3f)
      },
      {
        QString("Default"),   // Default part color names
        QString("Refrain"),
        QString("Bridge"),
        QString("Intro"),
        QString("Coda"),
        QString("Chorus"),
        QString("Solo"),
        QString("Brass"),
        QString("Percussion"),
        QString("Drums"),
        QString("Guitar"),
        QString("Bass"),
        QString("Flute"),
        QString("Strings"),
        QString("Keyboard"),
        QString("Piano"),
        QString("Saxophone")
      },
      QColor(51, 114, 178),   // transportHandleColor;
      QColor(219, 65, 65),    // bigTimeForegroundColor;
      QColor(0, 0, 0),        // bigTimeBackgroundColor;
      QColor(200, 192, 171),  // waveEditBackgroundColor;
      {
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 7,  QFont::Normal),    // Mixer strips and midi track info panel
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 10, QFont::Bold),
        QFont(QString("arial"), 8,  QFont::Normal),    // Small numbers: Timescale and markers, part name overlay
        QFont(QString("arial"), 8,  QFont::Bold),      // Small bold numbers such as marker text
        QFont(QString("arial"), 8,  QFont::Bold, true)  // Mixer strip labels. Looks and fits better with bold + italic than bold alone, 
                                                        //  at the price of only few more pixels than Normal mode.
        },
      QColor(84, 97, 114),          // trackBg;
      QColor(109, 174, 178),        // selected track Bg;
      QColor(0x00, 0x00, 0x00),     // selected track Fg;
      Qt::gray,                     // trackSectionDividerColor;
      
      QColor(74, 150, 194),         // midiTrackLabelBg;   // Med blue
      QColor(150, 177, 189),        // drumTrackLabelBg;   // Pale pastel blue
      QColor(116, 232, 242),        // newDrumTrackLabelBg; // Light blue
      QColor(213, 128, 202),        // waveTrackLabelBg;   // magenta
      QColor(84, 185, 58),          // outputTrackLabelBg; // green
      QColor(199, 75, 64),          // inputTrackLabelBg;  // red
      QColor(236, 214, 90),         // groupTrackLabelBg;  // yellow
      QColor(142, 157, 6),          // auxTrackLabelBg;    // Med olive
      QColor(229, 157, 101),        // synthTrackLabelBg;  // Med orange
      
      QColor(215, 220, 230),     // midiTrackBg;
      QColor(215, 220, 230),     // drumTrackBg;
      QColor(215, 220, 230),     // newDrumTrackBg;
      QColor(220, 209, 217),     // waveTrackBg;
      QColor(197, 220, 206),     // outputTrackBg;
      QColor(220, 214, 206),     // inputTrackBg;
      QColor(220, 216, 202),     // groupTrackBg;
      QColor(208, 215, 220),     // auxTrackBg;
      QColor(220, 211, 202),     // synthTrackBg;
      
      QColor(98, 124, 168),      // part canvas bg
      QColor(255, 170, 0),       // ctrlGraphFg;    Medium orange
      QColor(0, 0, 0),           // mixerBg;

      QColor(0xe0, 0xe0, 0xe0),     // Ruler background
      QColor(0, 0, 0),              // Ruler text
      QColor(255, 255, 255),        // Midi editor canvas
      QColor(255, 255, 255),        // midiControllerViewBg
      QColor(255, 255, 255),        // drumListBg
      QColor(255, 255, 255),        // rulerCurrent
      Qt::gray,                     // midiCanvasBeatColor
      Qt::black,                // midiCanvasBarColor

      Qt::lightGray,                // waveNonselectedPart
      Qt::darkGray,                 // wavePeakColor
      Qt::black,                    // waveRmsColor
      Qt::lightGray,                // wavePeakColorSelected
      Qt::white,                    // waveRmsColorSelected

      Qt::darkGray,                 // partWaveColorPeak
      QColor(20,20,20),             // partWaveColorRms
      QColor(54,54,54),             // partMidiDarkEventColor
      QColor(200,200,200),          // partMidiLightEventColor

      384,                          // division;
      1024,                         // rtcTicks
      true,                         // midiSendInit Send instrument initialization sequences
      true,                         // warnInitPending Warn instrument initialization sequences pending
      false,                        // midiSendCtlDefaults Send instrument controller defaults at position 0 if none in song
      true,                         // warnIfBadTiming Warn if timer res not good
      false,                        // velocityPerNote Whether to show per-note or all velocities
      -60,                          // int minMeter;
      -60.0,                        // double minSlider;
      false,                        // use Jack freewheel
      20,                           // int guiRefresh;
      QString(""),                  // userInstrumentsDir  // Obsolete. Must keep for compatibility.
      //QString(""),                // helpBrowser; // Obsolete
      true,                         // extendedMidi
      384,                          // division for smf export
      QString(""),                  // copyright string for smf export
      1,                            // smf export file format
      false,                        // midi export file 2 byte timesigs instead of 4
      true,                         // optimize midi export file note offs
      true,                         // Split imported tracks into multiple parts.
      true,                         // importMidiNewStyleDrum
      true,                         // importDevNameMetas Import Prefer Device Name metas over port number metas if both exist.
      true,                         // importInstrNameMetas Import Prefer Instrument Name metas over Mode sysexes if both exist.
      EXPORT_PORTS_DEVICES_ALL,     // exportPortsDevices Export port number metas and/or device name metas.
      true,                         // exportPortDeviceSMF0 Export a port and/or device meta even for SMF0.
      EXPORT_MODE_INSTR_ALL,        // exportModeInstr Export mode sysexes and/or instrument name metas.
      QString("GM"),                // importMidiDefaultInstr Default to this instrument not Generic, if no match found
      1,                            // startMode
      QString(""),                  // start song path
      false,                        // startSongLoadConfig
      384,                          // gui division
      QRect(0, 0, 400, 300),        // GeometryMain;
      QRect(0, 0, 200, 100),        // GeometryTransport;
      QRect(0, 0, 600, 200),        // GeometryBigTime;
      {
         QString("Mixer A"),
         QRect(0, 0, 300, 500),        // Mixer1
         true, true, true, true,
         true, true, true, true, true
         },
      {
         QString("Mixer B"),
         QRect(200, 200, 300, 500),    // Mixer2
         true, true, true, true,
         true, true, true, true, true
         },
      true,                         // TransportVisible;
      false,                        // BigTimeVisible;
      false,                        // mixer1Visible;
      false,                        // mixer2Visible;
      false,                        // markerVisible;
      true,                         // arrangerVisible;
      true,                         // showSplashScreen
      1,                            // canvasShowPartType 1 - names, 2 events
      5,                            // canvasShowPartEvent
      true,                         // canvasShowGrid;
      QString(""),                  // canvasBgPixmap;
      QStringList(),                // canvasCustomBgList
      QString(""),                  // default styleSheetFile - For built-in set to ":/style.qss"
      QString(""),                  // style
      QString("sweep"),             // externalWavEditor
      false,                        // useOldStyleStopShortCut
      false,                        // moveArmedCheckBox
      true,                         // useDenormalBias
      false,                        // useOutputLimiter
      true,                         // showDidYouKnow
      false,                        // vstInPlace  Enable VST in-place processing
      44100,                        // Dummy audio preferred sample rate
      512,                          // Dummy audio buffer size
      QString("./"),                // projectBaseFolder
      true,                         // projectStoreInFolder
      true,                         // useProjectSaveDialog
      256,                          // minControlProcessPeriod
      false,                        // popupsDefaultStayOpen
      false,                        // leftMouseButtonCanDecrease
      false,                        // rangeMarkerWithoutMMB
      MusECore::DONT_REC_MUTED_OR_HIDDEN,
      true,                         // addHiddenTracks
      true,                         // unhideTracks
      MusEGlobal::PREFER_NEW,       // drumTrackPreference
      true,                         // smartFocus
      20,                           // trackHeight
      true,                         // borderlessMouse
      false,                        // autoSave
      false,                        // scrollableSubMenus
      true,                         // liveWaveUpdate
      true,                         // warnOnFileVersions Warn if file version different than current
      QString("klick1.wav"),        // measSample
      QString("klick2.wav"),        // beatSample
      QString("klick3.wav"),        // accent1Sample
      QString("klick4.wav"),        // accent2Sample
      RoutePreferCanonicalName,     // preferredRouteNameOrAlias
      false,                        // routerExpandVertically
      2                             // routerGroupingChannels
    };

} // namespace MusEGlobal
