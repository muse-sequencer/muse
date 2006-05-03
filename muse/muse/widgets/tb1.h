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

#ifndef __TB1_H__
#define __TB1_H__

namespace Awl {
      class PosLabel;
      class PitchLabel;
      };
namespace AL {
      class Pos;
      };

class RasterCombo;
class QuantCombo;

enum { RANGE_ALL, RANGE_SELECTED, RANGE_LOOPED };

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

class Toolbar1 : public QToolBar {
      QToolButton* solo;
      Awl::PosLabel* pos;
      Awl::PitchLabel* pitch;
      QuantCombo* quant;
      QComboBox* toList;
      RasterCombo* raster;
      Q_OBJECT

   public slots:
      void setTime(const AL::Pos&, bool);
      void setPitch(int);
      void setInt(int);
      void setRaster(int);
      void setQuant(int);

   signals:
      void rasterChanged(int);
      void quantChanged(int);
      void soloChanged(bool);
      void toChanged(int);

   public:
      Toolbar1(int r=96, int q=96, bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);
      void setApplyTo(int);
      };

#endif

