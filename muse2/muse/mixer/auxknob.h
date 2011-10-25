//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: auxknob.h,v 1.3 2003/11/08 15:10:18 wschweer Exp $
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

#ifndef __AUXKNOB_H__
#define __AUXKNOB_H__

#include "knob.h"

namespace MusEGui {

//---------------------------------------------------------
//   AuxKnob
//---------------------------------------------------------

class AuxKnob : public Knob {
      Q_OBJECT
      int idx;

   private slots:
      void valueChanged(double v);

   signals:
      void auxChanged(int, double);

   public:
      AuxKnob(QWidget* parent, int idx);
      };

} // namespace MusEGui

#endif

