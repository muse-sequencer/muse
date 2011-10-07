//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale_flo.h,v 1.3 2011/05/19 22:27:06 flo Exp $
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

#ifndef __MTSCALE_FLO_H__
#define __MTSCALE_FLO_H__

#include "view.h"


namespace MusEGui {

class ScoreCanvas;

//---------------------------------------------------------
//   MTScaleFlo
//    scale for midi track
//---------------------------------------------------------

class MTScaleFlo : public View {
      Q_OBJECT
      unsigned pos[3];
      int button;
      ScoreCanvas* parent;
      int xpos;
      int xoffset;

   private slots:
      void songChanged(int);

   protected:
      virtual void draw(QPainter&, const QRect&);
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void mouseReleaseEvent(QMouseEvent* event);

   signals:
      void timeChanged(unsigned);

   public slots:
      void setPos(int, unsigned, bool);
      void set_xpos(int);
      void pos_add_changed();
      void set_xoffset(int);

   public:
      MTScaleFlo(ScoreCanvas* parent_editor, QWidget* parent_widget);
      };

} // namespace MusEGui

#endif

