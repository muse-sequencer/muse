//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: panknob.cpp,v 1.5 2004/01/23 08:41:38 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

//#include "../audio.h"
#include "song.h"
#include "panknob.h"
#include "track.h"

namespace MusEGui {

//---------------------------------------------------------
//   PanKnob
//---------------------------------------------------------

PanKnob::PanKnob(QWidget* parent, AudioTrack* s)
   : MusEGui::Knob(parent, "pan")
      {
      src = s;
      connect(this, SIGNAL(valueChanged(double,int)), SLOT(valueChanged(double)));
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void PanKnob::valueChanged(double val)
      {
      //audio->msgSetPan(src, val);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      src->setPan(val);
      }

} // namespace MusEGui
