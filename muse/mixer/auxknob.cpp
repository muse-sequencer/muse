//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: auxknob.cpp,v 1.7 2004/07/11 16:26:46 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#include <cmath>
#include "auxknob.h"
#include "gconfig.h"

namespace MusEGui {

//---------------------------------------------------------
//   Aux
//---------------------------------------------------------

AuxKnob::AuxKnob(QWidget* parent, int i)
   : MusEGui::Knob(parent, "aux")
      {
      idx = i;
      setRange(MusEGlobal::config.minSlider-0.1, 10.0);
      connect(this, SIGNAL(valueChanged(double,int)), SLOT(valueChanged(double)));
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AuxKnob::valueChanged(double val)
      {
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      emit auxChanged(idx, vol);
      }

} // namespace MusEGui
