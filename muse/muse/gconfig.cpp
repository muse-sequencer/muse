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

#include "gconfig.h"

GlobalConfigValues config = {
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
        QColor(0xff, 0xff, 0xff),
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
	QColor(100, 100, 100),  // selected Part Bg
	QColor(0, 0, 255),      // transportHandleColor;
	QColor(255, 0, 0),      // bigTimeForegroundColor;
	QColor(0, 0, 0),        // bigTimeBackgroundColor;
      QColor(200, 200, 200),  // waveEditBackgroundColor;
      {
        0, 0, 0, 0, 0, 0
        },
      {
            QColor(Qt::white),            // outputTrackBg;
            QColor(Qt::yellow),           // groupTrackBg;
            QColor(Qt::cyan),             // auxTrackBg;
            QColor(Qt::green),            // waveTrackBg;
            QColor(Qt::red),              // inputTrackBg;
            QColor(Qt::blue),             // synthTrackBg;
            QColor(Qt::gray),             // midi;
            QColor(Qt::gray),             // midiOut;
            QColor(Qt::gray),             // midiIn;
            QColor(Qt::gray),             // midiChannel;
            QColor(Qt::gray),             // midiSynti;
            },

      QColor(0, 0, 0),              // mixerBg;

      384,                          // division;
      1024,                         // rtcTicks
      -60,                          // int minMeter;
      -60.0,                        // double minSlider;
      20,                           // int guiRefresh;
      2000,					// peak hold time (ms)
      QString(""),                  // helpBrowser
      true,                         // extendedMidi
      384,                          // division for smf export
      QString(""),                  // copyright string for smf export
      1,                            // smf export file format
      START_ASK_FOR_PROJECT,        // startMode
      QString(""),                  // start song path
      384,                          // gui division
      QRect(0, 0, 800, 560),        // GeometryMain;
      QRect(0, 0, 200, 100),        // GeometryTransport;
      QRect(0, 0, 600, 200),        // GeometryBigTime;
      QRect(100, 100, 600, 400),    // GeometryPianoroll;
      QRect(0, 0, 600, 400),        // GeometryDrumedit;
      {
         QRect(0, 0, 300, 500),        // Mixer1
         false, true, true, true, true,
         true, true, true, true, true, true
         },
      {
         QRect(200, 200, 300, 500),    // Mixer2
         false, true, true, true, true,
         true, true, true, true, true, true
         },
      false,                        // TransportVisible
      false,                        // BigTimeVisible;
      false,                        // mixerVisible1;
      false,                        // mixerVisible2;

      true,                         // showSplashScreen

      QColor(0x71, 0x8d, 0xbe),     // canvasBgColor
      QString(""),                  // canvasBgPixmap;
      false,                        // canvasUseBgPixmap;
      1,                            // canvasShowPartType 1 - names, 2 events
      5,                            // canvasShowPartEvent
      false,                        // canvasShowGrid;

      QString(""),                  // style
      false,                        // use JACK freewheel mode
      QString("sweep"),             // externalWavEditor shell command

      QString(""),                  // defaultMidiInputDevice
      QString(""),                  // defaultMidiOutputDevice
      QString(""),                  // defaultMidiInstrument
      true,                         // connectToAllMidiDevices
      true,                         // connectToAllMidiTracks
      true,                         // createDefaultMidiInput
      QString("MusE/projects"),     // projectPath
      };

