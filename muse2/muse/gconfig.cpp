//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: gconfig.cpp,v 1.15.2.13 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "gconfig.h"

GlobalConfigValues config = {
      190,                        // globalAlphaBlend    
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
      QColor(0, 0, 255),      // transportHandleColor;
      QColor(255, 0, 0),      // bigTimeForegroundColor;
      QColor(0, 0, 0),        // bigTimeBackgroundColor;
      QColor(200, 200, 200),  // waveEditBackgroundColor;
      {
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 8,  QFont::Normal),
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 10, QFont::Bold),
        QFont(QString("arial"), 8,  QFont::Bold),    // timescale numbers
        QFont(QString("Lucidatypewriter"), 14,  QFont::Bold),
        QFont(QString("arial"), 8,  QFont::Bold, true)  // Mixer strip labels. Looks and fits better with bold + italic than bold alone, 
                                                        //  at the price of only few more pixels than Normal mode.
        },
      QColor(84, 97, 114),     // trackBg;
      QColor(0x80, 0xff, 0x80),     // selected track Bg;
      QColor(0x00, 0x00, 0x00),     // selected track Fg;
      
      QColor(0, 160, 255),          // midiTrackLabelBg;   // Med blue
      QColor(0, 160, 255),          // drumTrackLabelBg;   // Med blue
      Qt::magenta,                  // waveTrackLabelBg;
      Qt::green,                    // outputTrackLabelBg;
      Qt::red,                      // inputTrackLabelBg;
      Qt::yellow,                   // groupTrackLabelBg;
      QColor(120, 255, 255),        // auxTrackLabelBg;    // Light blue
      QColor(255, 130, 0),          // synthTrackLabelBg;  // Med orange
      
      QColor(220, 220, 220),     // midiTrackBg;
      QColor(220, 220, 220),     // drumTrackBg;
      QColor(220, 220, 220),     // waveTrackBg;
      QColor(189, 220, 193),     // outputTrackBg;
      QColor(189, 220, 193),     // inputTrackBg;
      QColor(220, 220, 220),     // groupTrackBg;
      QColor(220, 220, 220),     // auxTrackBg;
      QColor(220, 220, 220),     // synthTrackBg;
      
      QColor(98, 124, 168),      // part canvas bg
      QColor(255, 170, 0),       // ctrlGraphFg;    Medium orange
      QColor(0, 0, 0),           // mixerBg;

      384,                          // division;
      1024,                         // rtcTicks
      -60,                          // int minMeter;
      -60.0,                        // double minSlider;
      false,                        // use Jack freewheel
      20,                           // int guiRefresh;
      QString(""),                  // userInstrumentsDir
      //QString(""),                  // helpBrowser; // Obsolete
      true,                         // extendedMidi
      384,                          // division for smf export
      QString(""),                  // copyright string for smf export
      1,                            // smf export file format
      false,                        // midi export file 2 byte timesigs instead of 4
      true,                         // optimize midi export file note offs
      true,                         // Split imported tracks into multiple parts.
      1,                            // startMode
      QString(""),                  // start song path
      384,                          // gui division
      QRect(0, 0, 400, 300),        // GeometryMain;
      QRect(0, 0, 200, 100),        // GeometryTransport;
      QRect(0, 0, 600, 200),        // GeometryBigTime;
      QRect(0, 0, 400, 300),        // GeometryPianoroll;
      QRect(0, 0, 400, 300),        // GeometryDrumedit;
      //QRect(0, 0, 300, 500),        // GeometryMixer;  // Obsolete
      {
         QString("Mixer A"),
         QRect(0, 0, 300, 500),        // Mixer1
         true, true, true, true,
         true, true, true, true
         },
      {
         QString("Mixer B"),
         QRect(200, 200, 300, 500),    // Mixer2
         true, true, true, true,
         true, true, true, true
         },
      true,                         // TransportVisible;
      false,                        // BigTimeVisible;
      false,                        // mixer1Visible;
      false,                        // mixer2Visible;
      
      false,                        // markerVisible;
      true,                         // showSplashScreen
      1,                            // canvasShowPartType 1 - names, 2 events
      5,                            // canvasShowPartEvent
      true,                        // canvasShowGrid;
      QString(""),                  // canvasBgPixmap;
      QStringList(),                // canvasCustomBgList
      QString(":/style.qss"),       // default styleSheetFile
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
      64,                           // minControlProcessPeriod
      false                         // popupsDefaultStayOpen
    };

