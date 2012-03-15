//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchedit.h,v 1.2 2004/01/09 17:12:54 wschweer Exp $
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

#ifndef __PITCHEDIT_H__
#define __PITCHEDIT_H__

#include "spinbox.h"

namespace MusEGui {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

class PitchEdit : public SpinBox {
      Q_OBJECT

      bool deltaMode;

   protected:
      virtual QString mapValueToText(int v);
      virtual int mapTextToValue(bool* ok);

   public:
      PitchEdit(QWidget* parent=0);
      void setDeltaMode(bool);
      };

} // namespace MusEGui

#endif
