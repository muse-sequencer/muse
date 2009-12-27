//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.h,v 1.12.2.10 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <qcolor.h>
#include <qfont.h>
#include <qstring.h>
#include <qrect.h>

#define NUM_PARTCOLORS 17
#define NUM_FONTS 7

//---------------------------------------------------------
//   GlobalConfigValues
//---------------------------------------------------------

struct GlobalConfigValues {
      QColor palette[16];
      QColor partColors[NUM_PARTCOLORS];
	QColor transportHandleColor;
	QColor bigTimeForegroundColor;
	QColor bigTimeBackgroundColor;
      QColor waveEditBackgroundColor;
      //QFont fonts[6];
      QFont fonts[NUM_FONTS];
      QColor trackBg;
      QColor selectTrackBg;
      QColor selectTrackFg;
      QColor midiTrackBg;
      QColor ctrlGraphFg;
      QColor drumTrackBg;
      QColor waveTrackBg;
      QColor outputTrackBg;
      QColor inputTrackBg;
      QColor groupTrackBg;
      QColor auxTrackBg;
      QColor synthTrackBg;
      QColor partCanvasBg;
      QColor mixerBg;

      int division;
      int rtcTicks;
      int minMeter;
      double minSlider;
      bool freewheelMode;
      int guiRefresh;
      QString helpBrowser;

      bool extendedMidi;      // extended smf format
      int midiDivision;       // division for smf export
      QString copyright;      // copyright string for smf export
      int smfFormat;          // smf export file type
      bool exp2ByteTimeSigs;  // Export 2 byte time sigs instead of 4 bytes
      bool expOptimNoteOffs;  // Save space by replacing note offs with note on velocity 0
      
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
      QRect geometryMixer;
      bool transportVisible;
      bool bigTimeVisible;
      bool mixerVisible;
      bool markerVisible;

      bool showSplashScreen;
      int canvasShowPartType;       // 1 - names, 2 events
      int canvasShowPartEvent;      //
      bool canvasShowGrid;
      QString canvasBgPixmap;
      QString style;

      QString externalWavEditor;
      bool useOldStyleStopShortCut;
      bool useDenormalBias;
      bool useOutputLimiter;
      bool showDidYouKnow;
      bool vstInPlace; // Enable VST in-place processing
      };

extern GlobalConfigValues config;

#endif

