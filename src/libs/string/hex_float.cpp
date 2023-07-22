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

#include "hex_float.h"

#ifndef HAVE_ISTRINGSTREAM_HEXFLOAT
#include <cstdlib>
#include <QByteArray>
#endif

#include <sstream>
#include <clocale>

// Forwards from header:
#include <QString>


namespace MusELib {

#ifndef HAVE_ISTRINGSTREAM_HEXFLOAT
QString hexfloatDecimalPoint = QString('.');
#endif

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
//==================================================================
// Check if C++ istringstream supports hexfloat formatting.
// As of summer 2023, only experimental clang versions support this.
// And C++ is still in the process of adding support.
//==================================================================
#ifdef HAVE_ISTRINGSTREAM_HEXFLOAT

  // If C++ istringstream supports hexfloat, use it.
  std::istringstream ss(s.toStdString());
  ss.imbue(std::locale("C"));
  double value = 0.0;
  ss >> value;
  if(ok)
    *ok = true;

#else // C++ istringstream does not support hexfloat

  // Is it a hex number?
  if(s.startsWith("0x", Qt::CaseInsensitive) ||
     s.startsWith("-0x", Qt::CaseInsensitive) ||
     s.startsWith("+0x", Qt::CaseInsensitive))
  {
    // Note that strtod is locale sensitive.
    // We DO NOT want locale influence here. Just standard "C" locale. This function is intended for file IO.
    // One way around this is to call thread-safe per-thread functions uselocale() etc. but they aren't found on some platforms (mingw).
    //
    // Instead of trying to set locale to match our data, try the opposite: Translate the data to match locale.
    //
    // From docs:
    //  Hexadecimal floating-point expression. It consists of the following parts:
    //    (optional) plus or minus sign
    //    0x or 0X
    //    nonempty sequence of hexadecimal digits optionally containing a decimal-point character
    //     (as determined by the current C locale) (defines significand)
    //    (optional) p or P followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 2)
    //
    // Therefore in case of hexfloats, the decimal point should be the only thing requiring alteration here.
    // Note that our hexfloatDecimalPoint is a QString.
    //
    // Replace the known '.' decimal point with the current locale decimal point:

    QString s2(s);
    s2.replace(QChar('.'), hexfloatDecimalPoint);
    const QByteArray ba = s2.toUtf8();

    const char *sc = ba.constData();
    char *end;
    const double rv = std::strtod(sc, &end);
    if(ok)
      // "The function sets the pointer pointed to by str_end to point to the character past the last character interpreted."
      *ok = (end - sc) == ba.size();

    return rv;
  }
  else // Not a hex number.
  {
    // Just use normal toDouble(). Does not care about locale. Uses 'C' locale.
    return s.toDouble(ok);
  }

#endif // HAVE_ISTRINGSTREAM_HEXFLOAT
}

} // namespace MusELib

