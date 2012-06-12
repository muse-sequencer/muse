//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: musewidgetsplug.cpp,v 1.9.2.9 2009/12/01 03:52:40 terminator356 Exp $
//  (C) Copyright 2001-2003 Werner Schweer (ws@seh.de)
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

// this file makes some of the MusE widgets available
// to QT-Designer

// #include <qwidgetplugin.h>
#include <QtCore/QtPlugin>                             // p4.0.2
#include <QtDesigner/QDesignerCustomWidgetInterface>   //
#include <QPixmap>

#include "poslabel.h"
#include "pitchedit.h"
#include "pitchlabel.h"
#include "sig.h"
#include "tempo.h"
#include "tempolabel.h"
#include "sigedit.h"
#include "slider.h"
#include "doublelabel.h"
#include "checkbox.h"
#include "combobox.h"
#include "gconfig.h"

int sampleRate = 44100;   // some dummy values to get things compiled/linked
int division   = 384;
int MusEGlobal::mtcType    = 0;
bool hIsB      = false;

static const char* vall[] = {
      "c","c#","d","d#","e","f","f#","g","g#","a","a#","h"
      };
static const char* valu[] = {
      "C","C#","D","D#","E","F","F#","G","G#","A","A#","H"
      };

MusEGlobal::GlobalConfigValues config = {
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
        QFont(QString("arial"), 7,  QFont::Normal),    // Mixer strips and midi track info panel
        QFont(QString("arial"), 10, QFont::Normal),
        QFont(QString("arial"), 10, QFont::Bold),
        QFont(QString("arial"), 8,  QFont::Normal),    // Small numbers: Timescale and markers, part name overlay
        QFont(QString("arial"), 8,  QFont::Bold),      // Small bold numbers such as marker text
        QFont(QString("arial"), 8,  QFont::Bold, true)  // Mixer strip labels. Looks and fits better with bold + italic than bold alone, 
                                                        //  at the price of only few more pixels than Normal mode.
        },
      QColor(0xff, 0xff, 0xff),     // trackBg;
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
      
      QColor(98, 124, 168),         // part canvas bg
      QColor(255, 170, 0),          // ctrlGraphFg; Medium orange
      QColor(98, 124, 168),         // mixerBg;

      384,                          // division;
      1024,                         // rtcTicks
      -60,                          // int minMeter;
      -60.0,                        // double minSlider;
      false,                        // use Jack freewheel
      20,                           // int guiRefresh;
      QString(""),                  // userInstrumentsDir  // Obsolete. Must keep for compatibility.
      //QString(""),                // helpBrowser  // Obsolete
      true,                         // extendedMidi
      384,                          // division for smf export
      QString(""),                  // copyright string for smf export
      1,                            // smf export file format
      false,                        // midi export file 2 byte timesigs instead of 4
      true,                         // optimize midi export file note offs
      true,                         // Split imported tracks into multiple parts.
      1,                            // startMode
      QString(""),                  // start song path
      false,                        // startSongLoadConfig
      384,                          // gui division
      QRect(0, 0, 400, 300),        // GeometryMain;
      QRect(0, 0, 200, 100),        // GeometryTransport;
      QRect(0, 0, 600, 200),        // GeometryBigTime;
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
      false,                        // markerVisible;  // This line was missing  2007-01-08 (willyfoobar)
      true,                         // arrangerVisible;
      true,                         // showSplashScreen
      1,                            // canvasShowPartType 1 - names, 2 events
      5,                            // canvasShowPartEvent
      false,                        // canvasShowGrid;
      QString(""),                  // canvasBgPixmap;
      QStringList(),                // canvasCustomBgList
      QString(""),                  // default styleSheetFile - For built-in set to ":/style.qss"
      QString(""),                  // style
      QString(""),                  // externalWavEditor //this line was missing 2007-01-08 (willyfoobar)
      false,                        // useOldStyleStopShortCut
      true,                         // moveArmedCheckBox
      true,                         // useDenormalBias
      false,                        // useOutputLimiter
      true,                         // showDidYouKnow
      false,                        // vstInPlace  Enable VST in-place processing
      44100,                        // Dummy audio preferred sample rate
      512                           // Dummy audio buffer size
      QString("./"),                // projectBaseFolder
      true,                         // projectStoreInFolder
      true,                         // useProjectSaveDialog
      256,                          // minControlProcessPeriod
      false,                        // popupsDefaultStayOpen
      false,                        // leftMouseButtonCanDecrease
      false,                        // rangeMarkerWithoutMMBCheckBox
      true,                         // addHiddenTracks
      true,                         // unhideTracks
      true                          // smartFocus
      };

//---------------------------------------------------------
//   pitch2string
//---------------------------------------------------------

QString pitch2string(int v)
      {
      if (v < 0 || v > 127)
            return QString("----");
      int octave = (v / 12) - 2;
      QString o;
      o.sprintf("%d", octave);
      int i = v % 12;
      QString s(octave < 0 ? valu[i] : vall[i]);
      if (hIsB) {
            if (s == "h")
                  s = "b";
            else if (s == "H")
                  s = "B";
            }
      return s + o;
      }


/* XPM */
static const char* slider_pixmap[]={
"22 22 50 1",
". c None",
"f c #004005",
"g c #004007",
"h c #004107",
"m c #004108",
"j c #00430a",
"E c #00501f",
"s c #005021",
"e c #014006",
"l c #024006",
"F c #095e34",
"D c #0b572a",
"k c #0b582b",
"n c #0f5328",
"u c #12562d",
"o c #155a35",
"p c #165c38",
"q c #165c39",
"i c #17501a",
"I c #175522",
"r c #18603f",
"N c #18795e",
"t c #187a60",
"R c #1e5a29",
"y c #22633d",
"O c #307755",
"B c #408262",
"v c #439191",
"G c #468667",
"d c #4c7a51",
"H c #4d8a6c",
"J c #569174",
"C c #599276",
"P c #5e967a",
"A c #63b1c2",
"V c #659477",
"Q c #659b80",
"S c #6da087",
"w c #70b2bc",
"x c #72b5c0",
"z c #74b7c3",
"K c #79a891",
"a c #7ea48a",
"T c #8cb4a0",
"L c #a3c3b3",
"M c #b7d8d1",
"U c #bedcd5",
"c c #c3d2c3",
"b c #f1f5f1",
"# c #ffffff",
"......................",
"......................",
"......................",
"......................",
"......................",
"......###a............",
"......#bcad...........",
"......#bcad...........",
"......#bcad...........",
"..efgh#bcaihhhhhhhjk..",
".lmnop#bcaippppppqrst.",
".huvwx#bcayxxxxxxzABC.",
".DEFGH#bcaIHHHHHHJKLM.",
"..NOPQ#bcaRQQQQQQSTU..",
"......#bcad...........",
"......#bcad...........",
"......#bcad...........",
"......VVVVd...........",
".......dddd...........",
"......................",
"......................",
"......................"};
static const char *posedit_pixmap[] = {
          "22 22 8 1",
          "  c Gray100",
          ". c Gray97",
          "X c #4f504f",
          "o c #00007f",
          "O c Gray0",
          "+ c none",
          "@ c Gray0",
          "# c Gray0",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "+OOOOOOOOOOOOOOOOOOOO+",
          "OOXXXXXXXXXXXXXXXXXXOO",
          "OXX.          OO OO  O",
          "OX.      oo     O    O",
          "OX.      oo     O   .O",
          "OX  ooo  oooo   O    O",
          "OX    oo oo oo  O    O",
          "OX  oooo oo oo  O    O",
          "OX oo oo oo oo  O    O",
          "OX oo oo oo oo  O    O",
          "OX  oooo oooo   O    O",
          "OX            OO OO  O",
          "OO..................OO",
          "+OOOOOOOOOOOOOOOOOOOO+",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++"
      };

static const char *pitchedit_pixmap[] = {
          "22 22 8 1",
          "  c Gray100",
          ". c Gray97",
          "X c #4f504f",
          "o c #00007f",
          "O c Gray0",
          "+ c none",
          "@ c Gray0",
          "# c Gray0",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "+OOOOOOOOOOOOOOOOOOOO+",
          "OOXXXXXXXXXXXXXXXXXXOO",
          "OXX.          OO OO  O",
          "OX.      o      O    O",
          "OX.      oo     O   .O",
          "OX       o o    O    O",
          "OX       o      O    O",
          "OX     o o      O    O",
          "OX    oooo      O    O",
          "OX     o        O    O",
          "OX              O    O",
          "OX            OO OO  O",
          "OO..................OO",
          "+OOOOOOOOOOOOOOOOOOOO+",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++",
          "++++++++++++++++++++++"
      };

//---------------------------------------------------------
//   MusEPlugin
//---------------------------------------------------------

class MusEPlugin : public QWidgetPlugin {
   public:
      MusEPlugin() {}
      ~MusEPlugin() {}
      QStringList keys() const;
      QWidget* create(const QString& key, QWidget* parent=0,
         const char* name = 0);
      QString group(const QString& key) const;
      QIcon iconSet(const QString& key) const;
      QString includeFile(const QString& key) const;
      QString toolTip(const QString& key) const;
      QString whatsThis(const QString& key) const;
      bool isContainer(const QString& key) const;
      };

//---------------------------------------------------------
//   keys
//---------------------------------------------------------

QStringList MusEPlugin::keys() const
      {
      QStringList list;
      list << QString("PosEdit")
           << QString("PitchEdit")
           << QString("PosLabel")
           << QString("PitchLabel")
           << QString("TempoLabel")
           << QString("TempoEdit")
           << QString("SigEdit")
           << QString("Slider")
           << QString("DoubleLabel")
           << QString("CheckBox")
           << QString("ComboBox")
      ;
      return list;
      }

//---------------------------------------------------------
//   create
//---------------------------------------------------------

QWidget* MusEPlugin::create(const QString& key, QWidget* parent,
   const char* name)
      {
      if (key == QString("PosEdit"))
            return new PosEdit(parent, name);
      else if (key == QString("PitchEdit"))
            return new PitchEdit(parent, name);
      else if (key == QString("PitchLabel"))
            return new PitchLabel(parent, name);
      else if (key == QString("PosLabel"))
            return new PosLabel(parent, name);
      else if (key == QString("TempoLabel"))
            return new TempoLabel(parent, name);
      else if (key == QString("TempoEdit"))
            return new TempoEdit(parent, name);
      else if (key == QString("SigEdit"))
            return new SigEdit(parent, name);
      else if (key == QString("Slider"))
            return new Slider(parent, name);
      else if (key == QString("DoubleLabel"))
            return new DoubleLabel(parent, name);
      else if (key == QString("CheckBox"))
            return new CheckBox(parent, -1, name);
      else if (key == QString("ComboBox"))
            return new ComboBox(parent, name);
      return 0;
      }

//---------------------------------------------------------
//   group
//---------------------------------------------------------

QString MusEPlugin::group(const QString& /*key*/) const
      {
      return QString("MusE");
      }

//---------------------------------------------------------
//   iconSet
//---------------------------------------------------------

QIcon MusEPlugin::iconSet(const QString& key) const
      {
      if (key == "PosEdit" || key == "PosLabel")
            return QIcon(QPixmap(posedit_pixmap));
      else if (key == "PitchEdit" || key == "PitchLabel")
            return QIcon(QPixmap(pitchedit_pixmap));
      else if (key == "TempoEdit" || key == "TempoLabel")
            return QIcon(QPixmap(pitchedit_pixmap));
      else if (key == "SigEdit")
            return QIcon(QPixmap(pitchedit_pixmap));
      else if (key == QString("Slider"))
            return QIcon(QPixmap(slider_pixmap));
//      else if (key == QString("CheckBox"))
//            return QIconSet(QPixmap(slider_pixmap));
//      else if (key == QString("ComboBox"))
//            return QIconSet(QPixmap(slider_pixmap));
      return QIcon();
      }

//---------------------------------------------------------
//   includeFile
//---------------------------------------------------------

QString MusEPlugin::includeFile(const QString& key) const
      {
      if (key == QString("PosEdit"))
            return QString("posedit.h");
      else if (key == QString("PitchEdit"))
            return QString("pitchedit.h");
      else if (key == QString("PitchLabel"))
            return QString("pitchlabel.h");
      else if (key == QString("PosLabel"))
            return QString("poslabel.h");
      else if (key == QString("TempoLabel"))
            return QString("tempolabel.h");
      else if (key == QString("TempoEdit"))
            return QString("tempolabel.h");
      else if (key == QString("SigEdit"))
            return QString("sigedit.h");
      else if (key == QString("Slider"))
            return QString("slider.h");
      else if (key == QString("DoubleLabel"))
            return QString("dentry.h");
      else if (key == QString("CheckBox"))
            return QString("checkbox.h");
      else if (key == QString("ComboBox"))
            return QString("combobox.h");
      return QString::null;
      }

//---------------------------------------------------------
//   toolTip
//---------------------------------------------------------

QString MusEPlugin::toolTip (const QString& key) const
      {
      if (key == QString("PosEdit"))
            return QString("midi time position editor");
      else if (key == QString("PitchEdit"))
            return QString("midi pitch spinbox");
      else if (key == QString("PitchLabel"))
            return QString("midi pitch label");
      else if (key == QString("PosLabel"))
            return QString("midi time position label");
      else if (key == QString("TempoLabel"))
            return QString("midi tempo label");
      else if (key == QString("TempoEdit"))
            return QString("midi tempo spinbox");
      else if (key == QString("SigEdit"))
            return QString("midi signature spinbox");
      else if (key == QString("Slider"))
            return QString("slider for double values");
      else if (key == QString("DoubleLabel"))
            return QString("entry/label for double values");
      else if (key == QString("CheckBox"))
            return QString("checkbox with id");
      else if (key == QString("ComboBox"))
            return QString("combobox with id");
      return QString::null;
      }

//---------------------------------------------------------
//   whatsThis
//---------------------------------------------------------

QString MusEPlugin::whatsThis (const QString& key) const
      {
      if (key == QString("PosEdit"))
            return QString("midi time position editor");
      else if (key == QString("PitchEdit"))
            return QString("midi pitch spinbox");
      else if (key == QString("PitchLabel"))
            return QString("midi pitch label");
      else if (key == QString("PosLabel"))
            return QString("midi time position label");
      else if (key == QString("TempoLabel"))
            return QString("midi tempo label");
      else if (key == QString("TempoEdit"))
            return QString("midi tempo spinbox");
      else if (key == QString("SigEdit"))
            return QString("midi signature spinbox");
      else if (key == QString("Slider"))
            return QString("slider for double values");
      else if (key == QString("DoubleLabel"))
            return QString("entry/label for double values");
      else if (key == QString("CheckBox"))
            return QString("checkbox with id");
      else if (key == QString("ComboBox"))
            return QString("combobox with id");
      return QString::null;
      }

//---------------------------------------------------------
//   isContainer
//---------------------------------------------------------

bool MusEPlugin::isContainer (const QString& /*key*/) const
      {
      return false;
      }

Q_EXPORT_PLUGIN(MusEPlugin)

