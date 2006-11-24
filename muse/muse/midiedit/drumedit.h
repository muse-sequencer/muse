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

      virtual void closeEvent(QCloseEvent*);
      QWidget* genToolbar(QWidget* parent);
      DrumCanvas* canvas() { return (DrumCanvas*)tcanvas; }

   private slots:
      void noteinfoChanged(NoteInfo::ValType type, int val);
      virtual void cmd(QAction*);
      void drumCmd(QObject* object);

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
