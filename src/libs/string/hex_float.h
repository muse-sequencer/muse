//=========================================================
//  MusE
//  Linux Music Editor
//    hex_float.h
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

#ifndef __HEX_FLOAT_H__
#define __HEX_FLOAT_H__

#include "config.h"

// Forward refs.
class QString;

namespace MusELib {

//====================================================================================
// NOTICE: These functions are for reading and writing decimal and hexfloat values.
// We also ensure that the standard 'C' locale is used when reading and writing values.
// We do NOT want localisation in song files, for maximum portability.
//====================================================================================

extern QString museStringFromDouble(double v);
extern double museStringToDouble(const QString &s, bool *ok = nullptr);

#ifndef HAVE_ISTRINGSTREAM_HEXFLOAT
extern QString hexfloatDecimalPoint;
#endif

} // namespace MusELib

#endif
