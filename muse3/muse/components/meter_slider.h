//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  meter_slider.h
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __METER_SLIDER_H__
#define __METER_SLIDER_H__

#include "globaldefs.h"
#include "slider.h"

class QPaintEvent;

namespace MusEGui {

class Meter;

//---------------------------------------------------------
//   MeterSlider
//---------------------------------------------------------

class MeterSlider : public Slider
{
  Q_OBJECT

  private:
    int _maxMeterChans;
    Meter** _meters;
    
  protected:
    virtual void paintEvent (QPaintEvent*);
    
  public:
    MeterSlider(QWidget *parent, const char *name = 0,
                Qt::Orientation orient = Qt::Vertical,
                ScalePos scalePos = None,
                int maxMeterChans = MusECore::MAX_CHANNELS,
                int grooveWidth = 8,
                QColor fillColor = QColor(100, 100, 255));
  
};

} // namespace MusEGui

#endif
