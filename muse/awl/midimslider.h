//=============================================================================
//  Awl
//  Audio Widget Library
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

#ifndef __AWLMIDIMSLIDER_H__
#define __AWLMIDIMSLIDER_H__

#include "slider.h"

namespace Awl {

//---------------------------------------------------------
//   MidiMeterSlider
//---------------------------------------------------------

class MidiMeterSlider : public Slider
      {
      Q_PROPERTY(int meterWidth READ meterWidth WRITE setMeterWidth)
      Q_OBJECT

      double meterval;
      int _meterWidth;

      virtual void paint(const QRect& r);

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

