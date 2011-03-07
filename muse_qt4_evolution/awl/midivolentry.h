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

#ifndef __AWLMIDIVOLENTRY_H__
#define __AWLMIDIVOLENTRY_H__

#include "floatentry.h"

namespace Awl {

//---------------------------------------------------------
//   MidiVolEntry
//---------------------------------------------------------

class MidiVolEntry : public FloatEntry {
      Q_OBJECT
      Q_PROPERTY(int max READ max WRITE setMax)

  	int _max;

   public:
      virtual void setValue(double);
   	void setMax(int val) { _max = val; }
      int max() const      { return _max; }
      MidiVolEntry(QWidget* parent);
      };
}

#endif

