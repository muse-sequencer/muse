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

#ifndef __WAVE_EDIT_H__
#define __WAVE_EDIT_H__

#include "midiedit/midieditor.h"

class PartList;
class WaveView;
class ScrollScale;
class SNode;

namespace Awl {
      class PosLabel;
      };
using Awl::PosLabel;


//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public Editor {
      Q_OBJECT

      PartList* _parts;
      Part* selPart;

      WaveView* view;
      QToolBar* tools;
      QToolBar* tb1;
      QAction* solo;
      PosLabel* pos1;
      PosLabel* pos2;

      static int _widthInit, _heightInit;

      virtual void keyPressEvent(QKeyEvent*);

      QMenu* menuFunctions, *select, *menuGain;

   private slots:
      void cmd(QAction*);
      void setTime(unsigned t);
      void soloChanged(SNode* s);
      void soloChanged(bool flag);

   public slots:
      void configChanged();

   public:
      WaveEdit(PartList*, bool);
      ~WaveEdit();
      PartList* parts() const { return _parts; }
      void read(QDomNode node);
      void write(Xml& xml) const;

      enum { CMD_MUTE=0, CMD_NORMALIZE, CMD_FADE_IN, CMD_FADE_OUT, CMD_REVERSE,
             CMD_GAIN_FREE, CMD_GAIN_200, CMD_GAIN_150, CMD_GAIN_75, CMD_GAIN_50, CMD_GAIN_25,
             CMD_EDIT_EXTERNAL,
             CMD_SELECT_ALL, CMD_SELECT_NONE };
      static int initWidth, initHeight;
      static const int INIT_WIDTH  = 650;
      static const int INIT_HEIGHT = 450;
      };

static const bool INIT_FOLLOW = false;

#endif

