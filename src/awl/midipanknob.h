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

#ifndef __AWLMIDIPANKNOB_H__
#define __AWLMIDIPANKNOB_H__

#include "knob.h"

namespace Awl {

//---------------------------------------------------------
//   MidiPanKnob
//!   MidiPan Knob entry widget
//
//!   This widget implements a centered floating point
//!   knob used to adjust the pan position in an audio
//!   mixer.
//---------------------------------------------------------

class MidiPanKnob : public Knob {
      Q_OBJECT

      virtual void valueChange() {
            emit valueChanged(_value + 64.0f, _id);
            }

   public slots:
      virtual void setValue(double v) {
            AbstractSlider::setValue(v - 64.0f);
            }
   public:
      MidiPanKnob(QWidget* parent = 0);
      virtual double value() const { return _value + 64.0f; }
      };
}

#endif

