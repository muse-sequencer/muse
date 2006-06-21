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

#ifndef __CTRL_EDITOR_H__
#define __CTRL_EDITOR_H__

#include "al/pos.h"

class Ctrl;
class TimeCanvas;
class Track;

//---------------------------------------------------------
//   CtrlEditor
//---------------------------------------------------------

class CtrlEditor {
      bool _drawCtrlName;
      int dragx, dragy;
      int dragYoffset;
      int startY;

      int lselected;    // cached pixel position of current value
      AL::Pos selected; // current selected controller value

      bool drawRuler;
      QPoint ruler1;
      QPoint ruler2;

      virtual Ctrl* ctrl()     const = 0;
      virtual TimeCanvas* tc() const = 0;
      virtual int cheight()    const = 0;
      virtual Track* track()   const = 0;
      virtual Track* ctrlTrack()   const = 0;

   protected:
      int singlePitch;

   public:
      CtrlEditor();
      virtual ~CtrlEditor() {}
      void paint(QPainter& p, const QRect& r);
      void setDrawCtrlName(bool val) { _drawCtrlName = val; }
      void mousePress(const QPoint&, int);
      void mouseRelease();
      void mouseMove(const QPoint& pos);
      };

#endif

