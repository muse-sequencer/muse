//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.h,v 1.12.2.10 2009/12/01 03:52:40 terminator356 Exp $
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NUM_PARTCOLORS 17
#define NUM_FONTS 7

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>

namespace MusECore {
class Xml;
}

namespace MusEGlobal {

//---------------------------------------------------------
//   MixerConfig
//---------------------------------------------------------

struct MixerConfig {
      QString name;
      QRect geometry;
      bool showMidiTracks;
      bool showDrumTracks;
      bool showInputTracks;
      bool showOutputTracks;
      bool showWaveTracks;
      bool showGroupTracks;
      bool showAuxTracks;
      bool showSyntiTracks;

      void write(int level, MusECore::Xml& xml);
      void read(MusECore::Xml& xml);
      };

//---------------------------------------------------------
//   GlobalConfigValues
//---------------------------------------------------------

struct GlobalConfigValues {
      int globalAlphaBlend;
      QColor palette[16];
      QColor partColors[NUM_PARTCOLORS];
      QString partColorNames[NUM_PARTCOLORS];
      QColor transportHandleColor;
      QColor bigTimeForegroundColor;
      QColor bigTimeBackgroundColor;
      QColor waveEditBackgroundColor;
      QFont fonts[NUM_FONTS];
      QColor trackBg;
      QColor selectTrackBg;
      QColor selectTrackFg;
      
      QColor midiTrackLabelBg;
      QColor drumTrackLabelBg;
      QColor waveTrackLabelBg;
      QColor outputTrackLabelBg;
      QColor inputTrackLabelBg;
      QColor groupTrackLabelBg;
      QColor auxTrackLabelBg;
      QColor synthTrackLabelBg;

      QColor midiTrackBg;
      QColor drumTrackBg;
      QColor waveTrackBg;
      QColor outputTrackBg;
      QColor inputTrackBg;
      QColor groupTrackBg;
      QColor auxTrackBg;
      QColor synthTrackBg;
      
      QColor partCanvasBg;
      QColor ctrlGraphFg;
      QColor mixerBg;

      int division;
      int rtcTicks;
      int minMeter;
      double minSlider;
      bool freewheelMode;
      int guiRefresh;
      QString userInstrumentsDir;  // Obsolete. Must keep for compatibility.

      bool extendedMidi;      // extended smf format
      int midiDivision;       // division for smf export
      QString copyright;      // copyright string for smf export
      int smfFormat;          // smf export file type
      bool exp2ByteTimeSigs;  // Export 2 byte time sigs instead of 4 bytes
      bool expOptimNoteOffs;  // Save space by replacing note offs with note on velocity 0
      bool importMidiSplitParts; // Split imported tracks into multiple parts.
      
      int startMode;          // 0 - start with last song
                              // 1 - start with default template
                              // 2 - start with song
      QString startSong;      // path for start song
      bool startSongLoadConfig;  // Whether to load configuration with the start template or song
      int guiDivision;        // division for tick display

      QRect geometryMain;
      QRect geometryTransport;
      QRect geometryBigTime;
      MixerConfig mixer1;
      MixerConfig mixer2;
      bool transportVisible;
      bool bigTimeVisible;
      bool mixer1Visible;
      bool mixer2Visible;
      bool markerVisible;
      bool arrangerVisible;

      bool showSplashScreen;
      int canvasShowPartType;       // 1 - names, 2 events
      int canvasShowPartEvent;      //
      bool canvasShowGrid;
      QString canvasBgPixmap;
      QStringList canvasCustomBgList;
      QString styleSheetFile;
      QString style;

      QString externalWavEditor;
      bool useOldStyleStopShortCut;
      bool moveArmedCheckBox;
      bool useDenormalBias;
      bool useOutputLimiter;
      bool showDidYouKnow;
      bool vstInPlace; // Enable VST in-place processing
      int dummyAudioSampleRate; 
      int dummyAudioBufSize;
      QString projectBaseFolder;
      bool projectStoreInFolder;
      bool useProjectSaveDialog;
      unsigned long minControlProcessPeriod;
      bool popupsDefaultStayOpen;
      bool leftMouseButtonCanDecrease;
      bool rangeMarkerWithoutMMB;
      bool addHiddenTracks;
      bool unhideTracks;
      bool smartFocus;

      };




extern GlobalConfigValues config;
} // namespace MusEGlobal

#endif

