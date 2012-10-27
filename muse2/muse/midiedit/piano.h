//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: piano.h,v 1.2 2004/05/31 11:48:55 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __PIANO_H__
#define __PIANO_H__

#include "view.h"

class QEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QPixmap;

#define KH  13

namespace MusEGui {
  
//---------------------------------------------------------
//   Piano
//---------------------------------------------------------

class Piano : public View
      {
      Q_OBJECT
    
      int curPitch;
      int _curSelectedPitch;
      QPixmap* octave;
      QPixmap* c_keys[10];
      QPixmap* mk1;
      QPixmap* mk2;
      QPixmap* mk3;
      QPixmap* mk4;
      QPixmap* mk5;
      QPixmap* mk6;
      QPixmap* mk7;
      QPixmap* mk8;
      int keyDown;
      bool shift;
      int button;

      
      int y2pitch(int) const;
      int pitch2y(int) const;
      void viewMouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent* e);

   protected:
      virtual void draw(QPainter&, const QRect&);

   signals:
      void pitchChanged(int);
      void keyPressed(int, int, bool);
      void keyReleased(int, bool);
      void curSelectedPitchChanged(int);
      void redirectWheelEvent(QWheelEvent*);

   public slots:
      void setPitch(int);

   public:
      Piano(QWidget*, int);
      int curSelectedPitch() const { return _curSelectedPitch; } 
      void setCurSelectedPitch(int pitch);
      };

} // namespace MusEGui
      
#endif

