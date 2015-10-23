//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigscale.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
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

#ifndef __SIGSCALE_H__
#define __SIGSCALE_H__

#include "view.h"

class MidiEditor;

namespace MusEGui {

//---------------------------------------------------------
//   SigScale
//    Time Signature Scale
//---------------------------------------------------------

class SigScale : public View {
      Q_OBJECT
      int* raster;
      unsigned pos[3];
      int button;

   signals:
      void posChanged(unsigned, unsigned);

   protected:
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   signals:
      void timeChanged(unsigned);

   public slots:
      void setPos(int, unsigned, bool);

   public:
      SigScale(int* raster, QWidget* parent, int xscale);
      };

} // namespace MusEGui

#endif

