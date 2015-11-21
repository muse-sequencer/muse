//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tscale.h,v 1.1.1.1 2003/10/27 18:52:36 wschweer Exp $
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

#ifndef __TSCALE_H__
#define __TSCALE_H__

#include "view.h"

namespace MusEGui {

//---------------------------------------------------------
//   Tscale
//---------------------------------------------------------

class TScale : public View {
      Q_OBJECT
        
      double curTempo;

      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   protected:
      virtual void pdraw(QPainter&, const QRect&);

   signals:
      void tempoChanged(int);

   public:
      TScale(QWidget*, int);
      };

} // namespace MusEGui

#endif

