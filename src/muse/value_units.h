//=========================================================
//  MusE
//  Linux Music Editor
//
//  value_units.h
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

#ifndef __VALUE_UNITS_H__
#define __VALUE_UNITS_H__

#include <QString>
#include <QStringList>

namespace MusECore {

class ValueUnits_t : public QStringList
{
  public:
    // Adds a symbol text. Returns the index.
    // If the symbol already exists, returns the index of the symbol.
    int addSymbol(const QString &s);
    // Returns the symbol at index, or empty string if index is < 0 or >= size.
    QString symbol(int idx) const;
};

} // namespace MusECore

#endif
