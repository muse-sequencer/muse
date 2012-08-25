//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale.h,v 1.3 2004/04/27 22:27:06 spamatica Exp $
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

#ifndef __MTSCALE_H__
#define __MTSCALE_H__

#include "type_defs.h"
#include "view.h"
#include "pos.h"
#include "rasterizer.h"

namespace MusEGui {

//---------------------------------------------------------
//   MTScale
//    scale for midi track
//---------------------------------------------------------

class MTScale : public View {
      Q_OBJECT
      //int* raster;
      MusECore::Rasterizer _rasterizer;
      //unsigned pos[4];
      MusECore::Pos pos[4];
      int button;
      bool barLocator;
      //bool waveMode;
      //MusECore::Pos::TType _timeType;

   private slots:
      void songChanged(MusECore::SongChangedFlags_t);

   protected:
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   signals:
      void timeChanged(unsigned tickPos);
      void timeChanged(MusECore::Pos pos);
      //void addMarker(int);

   public slots:
      //void setPos(int, unsigned, bool);
      void setPos(int idx, const MusECore::Pos& pos, bool adjustScrollbar);
      void setTimeType(MusECore::Pos::TType tt);
      void setFormatted(bool f);

   public:
      //MTScale(int* raster, QWidget* parent, int xscale, bool f = false);
      //MTScale(int* raster, QWidget* parent, int xscale, MusECore::Pos::TType time_type = MusECore::Pos::TICKS);
      MTScale(QWidget* parent, int xscale, const MusECore::Rasterizer& rasterizer);
      void setBarLocator(bool f) { barLocator = f; }
      //MusECore::Pos::TType timeType() const { return _timeType; }
      //MusECore::Pos::TType timeType() const { return _rasterizer.timeType(); }
      //bool formatted() const { return _rasterizer.formatted(); }
      const MusECore::Rasterizer& rasterizer() const { return _rasterizer; } 
      };

} 

#endif

