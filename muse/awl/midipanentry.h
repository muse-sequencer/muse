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

#ifndef __AWLMIDIPANENTRY_H__
#define __AWLMIDIPANENTRY_H__

#include "floatentry.h"

namespace Awl {

//---------------------------------------------------------
//   MidiPanEntry
//---------------------------------------------------------

class MidiPanEntry : public FloatEntry {
      Q_OBJECT

   protected:
      virtual void valueChange();

   public slots:
      virtual void setValue(float v) {
            FloatEntry::setValue(v - 64.0f);
            }
   public:
      MidiPanEntry(QWidget* parent);
      virtual float value() const  { return _value + 64.0f; }
      };
}

#endif
