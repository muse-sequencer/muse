//=========================================================
//  MusE
//  Linux Music Editor
//    hex_float.cpp
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

#include <sstream>
#include <QString>
#include "hex_float.h"

namespace MusELib {
QString museStringFromDouble(double v)
{
    std::stringstream ss;
    ss.precision(100);
    ss.imbue(std::locale("C"));
    ss << v;
    if (ss.str().size() > 10) {
        ss.str("");
        ss << std::hexfloat << v;
    }
    return QString::fromLatin1(ss.str().c_str());
}

double museStringToDouble(const QString &s, bool *ok)
{
    std::stringstream ss;
    double value;
    size_t end;
    ss.imbue(std::locale("C"));
    ss << s.toStdString();
    value = std::stod(ss.str(), &end);
    *ok = (s.size() == end);
    return value;
}
} // namespace MusELib

