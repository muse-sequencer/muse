//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer
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

#ifndef __SPLITLAYERGUI_H__
#define __SPLITLAYERGUI_H__

#include "awl/pitchedit.h"

class SplitLayer;

//---------------------------------------------------------
//   SplitLayerGui
//---------------------------------------------------------

class SplitLayerGui : public QWidget {
      Q_OBJECT
      SplitLayer* sl;

      Awl::PitchEdit* p1[16];
      Awl::PitchEdit* p2[16];
      Awl::PitchEdit* p3[16];
      QSpinBox* p4[16];
      QSpinBox* p5[16];
      QSpinBox* p6[16];

      QAction* a1[16];
      QAction* a2[16];
      int fd1, fd2;

   private slots:
      void learnStartPitch(QAction*);
      void learnEndPitch(QAction*);
      void startPitchChanged(int);
      void endPitchChanged(int);
      void pitchOffsetChanged(int);
      void resetLearnMode(int);
      void startVeloChanged(int);
      void endVeloChanged(int);
      void veloOffsetChanged(int);

   public:
      SplitLayerGui(SplitLayer*, QWidget* parent=0);
      void init();
      void sendResetLearnMode();
      };

#endif

