//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __AWLMIDIMSLIDER_H__
#define __AWLMIDIMSLIDER_H__

#include "slider.h"

namespace Awl {

//---------------------------------------------------------
//   MidiMeterSlider
//---------------------------------------------------------

class MidiMeterSlider : public Slider
      {
      Q_OBJECT
      Q_PROPERTY(int meterWidth READ meterWidth WRITE setMeterWidth)
      

      double meterval;
      int _meterWidth;

      virtual void paintEvent(QPaintEvent*);

   protected:
      virtual void mouseDoubleClickEvent(QMouseEvent*);

   public slots:
      void setMeterVal(double value);

   public:
      MidiMeterSlider(QWidget* parent = 0);
      int meterWidth() const    { return _meterWidth; }
      void setMeterWidth(int v) { _meterWidth = v; }
      };
}

#endif

