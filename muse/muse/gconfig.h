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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NUM_PARTCOLORS 17

#include "track.h"
#include "al/xml.h"
using AL::Xml;

//---------------------------------------------------------
//   MixerConfig
//---------------------------------------------------------

struct MixerConfig {
      QRect geometry;
      bool showMidiTracks;
      bool showMidiInPorts;
      bool showMidiSyntiPorts;
      bool showMidiOutPorts;
      bool showMidiChannels;
      bool showOutputTracks;
      bool showWaveTracks;
      bool showGroupTracks;
      bool showInputTracks;
      bool showAuxTracks;
      bool showSyntiTracks;

      void write(Xml&, const char* name);
      void read(QDomNode);
      };

//---------------------------------------------------------
//   GlobalConfigValues
//---------------------------------------------------------

struct GlobalConfigValues {
      QColor palette[16];
      QColor partColors[NUM_PARTCOLORS];
      QColor selectPartBg;
	QColor transportHandleColor;
	QColor bigTimeForegroundColor;
	QColor bigTimeBackgroundColor;
      QColor waveEditBackgroundColor;
      QFont* fonts[6];

      QColor trackBg[Track::TRACK_TYPES];

      QColor mixerBg;

      int division;
      int rtcTicks;
      int minMeter;
      double minSlider;
      int guiRefresh;
      int peakHoldTime;		// peak meter hold time (ms)
      QString helpBrowser;

      bool extendedMidi;      // extended smf format
      int midiDivision;       // division for smf export
      QString copyright;      // copyright string for smf export
      int smfFormat;          // smf export file type

      int startMode;          // 0 - start with last song
                              // 1 - start with default template
                              // 2 - start with song
      QString startSong;      // path for start song
      int guiDivision;        // division for tick display

      QRect geometryMain;
      QRect geometryTransport;
      QRect geometryBigTime;
      QRect geometryPianoroll;
      QRect geometryDrumedit;
      MixerConfig mixer1;
      MixerConfig mixer2;
      bool transportVisible;
      bool bigTimeVisible;
      bool mixer1Visible;
      bool mixer2Visible;

      bool showSplashScreen;

      QColor canvasBgColor;
      QString canvasBgPixmap;
      bool canvasUseBgPixmap;

      int canvasShowPartType;       // 1 - names, 2 events
      int canvasShowPartEvent;      //
      bool canvasShowGrid;

      QString style;

      bool useJackFreewheelMode;
      QString externalWavEditor;

      QString defaultMidiInputDevice;
      QString defaultMidiOutputDevice;
      QString defaultMidiInstrument;
      bool connectToAllMidiDevices;
      bool connectToAllMidiTracks;
      bool createDefaultMidiInput;
      };

extern GlobalConfigValues config;

#endif

