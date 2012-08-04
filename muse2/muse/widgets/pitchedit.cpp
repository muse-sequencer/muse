//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchedit.cpp,v 1.2 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include "pitchedit.h"
#include "helper.h"

namespace MusEGui {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

PitchEdit::PitchEdit(QWidget* parent)
  : SpinBox(parent)
      {
      setMinimum(0);
      setMaximum(127);
      setSingleStep(1);
      deltaMode = false;
      }

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString PitchEdit::mapValueToText(int v)
      {
      if (deltaMode) {
            QString s;
            s.setNum(v);
            return s;
            }
      else
            return MusECore::pitch2string(v);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int PitchEdit::mapTextToValue(bool* ok)
      {
      printf("PitchEdit: mapTextToValue: not impl.\n");
      if (ok)
            *ok = false;
      return 0;
      }

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void PitchEdit::setDeltaMode(bool val)
      {
      if(deltaMode == val)
        return;
      
      deltaMode = val;
      if (deltaMode)
            setRange(-127, 127);
      else
            setRange(0, 127);
      }

} // namespace MusEGui
