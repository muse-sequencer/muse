//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.h,v 1.40 2006/02/08 17:33:41 wschweer Exp $
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DRUM_EDIT_H__
#define __DRUM_EDIT_H__

#include "midieditor.h"
#include "dcanvas.h"

namespace AL {
      class Xml;
      };
using AL::Xml;


class PartList;
class Part;
class DrumMap;

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

class DrumEdit : public MidiEditor {
      Q_OBJECT

      Event selEvent;
      DrumMap* drumMap;
      int selTick;
      QMenu *menuFunctions, *menuSelect;

      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;

      QAction* cmdActions[DrumCanvas::CMD_COMMANDS];

      void initShortcuts();
      virtual void closeEvent(QCloseEvent*);
      QWidget* genToolbar(QWidget* parent);
      virtual void keyPressEvent(QKeyEvent*);
      DrumCanvas* canvas() { return (DrumCanvas*)tcanvas; }

   private slots:
      void noteinfoChanged(NoteInfo::ValType type, int val);
      virtual void cmd(QAction*);
      void configChanged();

   public slots:
      void setSelection(int, Event&, Part*);
      void soloChanged(bool);       // called by Solo button

   public:
      DrumEdit(PartList*, bool);
      ~DrumEdit();

      static int initRaster, initQuant, initWidth, initHeight;
      static bool initFollow, initSpeaker, initMidiin;
      static int initApplyTo;
      static double initXmag;

      static void readConfiguration(QDomNode);
      static void writeConfiguration(Xml&);

      static const int INIT_WIDTH    = 650;
      static const int INIT_HEIGHT   = 450;
      static const int INIT_RASTER   = 384 / 4;
      static const int INIT_QUANT    = 384 / 4;
      static const bool INIT_FOLLOW  = false;
      static const bool INIT_SPEAKER = true;
      static const bool INIT_SREC    = false;
      static const bool INIT_MIDIIN  = false;
      static const double INIT_XMAG  = 0.08;
      static const int INIT_APPLY_TO = 0;
      };

#endif
