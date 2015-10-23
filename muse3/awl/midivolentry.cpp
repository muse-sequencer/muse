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
#include "midivolentry.h"

namespace Awl {

//---------------------------------------------------------
//   MidiVolEntry
//---------------------------------------------------------

MidiVolEntry::MidiVolEntry(QWidget* parent)
   : FloatEntry(parent)
      {
  	_max = 127;
      setRange(-98.0f, 0.0f);
      setSpecialText(tr("off"));
      setSuffix(tr("db"));
      setFrame(true);
      setPrecision(0);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void MidiVolEntry::setValue(double v)
      {
      FloatEntry::setValue(-fast_log10(double(_max*_max)/(v*v))*20.0f);
      }
}

