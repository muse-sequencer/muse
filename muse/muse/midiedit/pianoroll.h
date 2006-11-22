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

#ifndef __PIANOROLL_H__
#define __PIANOROLL_H__

#include "midieditor.h"
#include "prcanvas.h"

namespace AL {
      class Xml;
      };
using AL::Xml;

class PianoCanvas;
class PartList;
class QuantConfig;
class Part;
class TimeCanvas;

//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

class PianoRoll : public MidiEditor {
      Q_OBJECT

      Q_PROPERTY(int colorMode     READ colorMode     WRITE setColorMode)
      Q_PROPERTY(int quantStrength READ quantStrength WRITE setQuantStrength)
      Q_PROPERTY(int quantLimit    READ quantLimit    WRITE setQuantLimit)
      Q_PROPERTY(bool quantLen     READ quantLen      WRITE setQuantLen)

      int _quantStrength, _quantLimit;
      bool _quantLen;
      int _colorMode;

      Event selEvent;

      enum { CMD_EVENT_COLOR, CMD_CONFIG_QUANT, CMD_LAST };

      QAction* menu_ids[CMD_LAST];
      QMenu *menuFunctions, *menuSelect, *menuConfig;

      int tickOffset;
      int lenOffset;
      int pitchOffset;
      int veloOnOffset;
      int veloOffOffset;
      bool deltaMode;

      QAction* colorModeAction[3];
      QMenu* eventColor;
      QuantConfig* quantConfig;

      QWidget* genToolbar(QWidget* parent);
      virtual void keyPressEvent(QKeyEvent*);
      void setEventColorMode(int);
      PianoCanvas* canvas() { return (PianoCanvas*)tcanvas; }
      const PianoCanvas* canvas() const { return (PianoCanvas*)tcanvas; }

   private slots:
      void setSelection(int, Event&, Part*);
      void noteinfoChanged(NoteInfo::ValType, int);
      void soloChanged(bool flag);
      void configQuant();

      virtual void cmd(QAction*);
      void setEventColorMode(QAction*);
      void configChanged();
      void cmdLeft();
      void cmdRight();

   public:
      PianoRoll(PartList*, bool);
      ~PianoRoll() {}

      int colorMode() const          { return _colorMode;     }
      void setColorMode(int val)     { _colorMode = val;      }
      int quantStrength() const      { return _quantStrength; }
      int quantLimit() const         { return _quantLimit;    }
      bool quantLen() const          { return _quantLen;      }
      void setQuantStrength(int val) { _quantStrength = val;  }
      void setQuantLimit(int val)    { _quantLimit = val;     }
      void setQuantLen(bool val)     { _quantLen = val;       }

      static void readConfiguration(QDomNode);
      static void writeConfiguration(Xml&);
      static int initRaster, initQuant, initWidth, initHeight;
      static bool initFollow, initSpeaker, initMidiin;
      static int initColorMode, initApplyTo;
      static double initXmag;
      static int initQuantStrength, initQuantLimit;
      static bool initQuantLen;

      static const int INIT_WIDTH       = 650;
      static const int INIT_HEIGHT      = 450;
      static const int INIT_RASTER      = 384 / 2;
      static const int INIT_QUANT       = 384 / 2;
      static const bool INIT_FOLLOW     = false;
      static const bool INIT_SPEAKER    = true;
      static const bool INIT_MIDIIN     = false;
      static const bool INIT_SREC       = false;
      static const bool INIT_COLOR_MODE = 0;
      static const double INIT_XMAG     = 0.08;
      static const int INIT_APPLY_TO       = 0;
      static const int INIT_QUANT_STRENGTH = 100;
      static const int INIT_QUANT_LIMIT    = 0;
      static const int INIT_QUANT_LEN      = false;
      };

#endif

