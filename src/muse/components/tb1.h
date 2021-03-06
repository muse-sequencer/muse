//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tb1.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
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

#ifndef __TB1_H__
#define __TB1_H__

#include <QToolBar>

#include "rasterizer.h"

class QToolButton;

namespace MusEGui {

class PitchLabel;
class PosLabel;
class RasterLabelCombo;

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

class Toolbar1 : public QToolBar {       
      Q_OBJECT
    
      QToolButton* solo;
      PosLabel* pos;
      PitchLabel* pitch;
      QToolButton* gridOnButton;
      RasterLabelCombo* raster;
      bool showPitch;

   private slots:
      void _rasterChanged(int raster);
      void gridOnButtonChanged(bool v);

   public slots:
      void setTime(unsigned);
      void setPitch(int);
      void setInt(int);
      void setRaster(int);
      void setGridOn(bool flag);

   signals:
      void rasterChanged(int raster);
      void soloChanged(bool);
      void gridOnChanged(bool v);

   public:
      Toolbar1(RasterizerModel *model, QWidget* parent, int r=96,     
         bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);

      const Rasterizer *rasterizer() const;
      void setRasterizerModel(RasterizerModel *model);
      int currentRaster() const;
      // Same as setRaster() but returns the actual value used.
      int changeRaster(int val);
      };

} // namespace MusEGui

#endif
