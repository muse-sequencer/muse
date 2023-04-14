//=========================================================
//  MusE
//  Linux Music Editor
//
//  value_units.cpp
//  (C) Copyright 2023 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "value_units.h"

namespace MusECore {

int ValueUnits_t::addSymbol(const QString &s)
{
  if(!s.isEmpty())
  {
    const int idx = indexOf(s);
    if(idx == -1)
    {
      append(s);
      return size() - 1;
    }
    else
      return idx;
  }
  return -1;
}

QString ValueUnits_t::symbol(int idx) const
{
  if(idx < 0 || idx >= size())
    return QString();
  return at(idx);
}

} // namespace MusECore
