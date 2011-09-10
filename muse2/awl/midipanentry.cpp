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

#include "fastlog.h"
#include "midipanentry.h"

namespace Awl {

//---------------------------------------------------------
//   MidiPanEntry
//---------------------------------------------------------

MidiPanEntry::MidiPanEntry(QWidget* parent)
   : FloatEntry(parent)
      {
      setRange(-64.0f, 63.0f);
      setFrame(true);
      setPrecision(0);
      }

//---------------------------------------------------------
//   valueChange
//---------------------------------------------------------

void MidiPanEntry::valueChange()
      {
      emit valueChanged(_value + 64.0f, _id);
      }
}

