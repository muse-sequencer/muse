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

class QToolButton;
class QTableWidget;

class Track;
class LabelCombo;

namespace MusEGui {

class PitchLabel;
class PosLabel;

//---------------------------------------------------------
//   Toolbar1
//---------------------------------------------------------

class Toolbar1 : public QToolBar {       
      Q_OBJECT
    
      QToolButton* solo;
      PosLabel* pos;
      PitchLabel* pitch;
      LabelCombo* raster;
      QTableWidget* rlist;
      bool showPitch;
      
   private slots:
      void _rasterChanged(int);

   public slots:
      void setTime(unsigned);
      void setPitch(int);
      void setInt(int);
      void setRaster(int);

   signals:
      void rasterChanged(int);
      void soloChanged(bool);

   public:
      //Toolbar1(QMainWindow* parent = 0, int r=96,
      Toolbar1(QWidget* parent, int r=96,     
         bool showPitch=true);
      void setSolo(bool val);
      void setPitchMode(bool flag);
      };

} // namespace MusEGui

#endif
