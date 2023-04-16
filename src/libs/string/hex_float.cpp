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

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <locale>

#include <QString>

#include "hex_float.h"

namespace MusELib {

QString museStringFromDouble(double v)
{
  // NOTE: snprintf can be locale sensitive! We should force the locale to standard 'C'.
  // Use a per-thread thread-safe technique instead of setting the global locale.
  // Note this C locale is NOT the same as the application's C++ locale.
  // Setting LANG environment variable before running is different than setting our -l switch.
  locale_t nloc = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
  assert(nloc != (locale_t) 0);
  uselocale(nloc);

  bool useh = false;
  int sz;
  // How many characters required in the decimal string?
  // Use a very large precision to get all the digits.
  sz = std::snprintf(nullptr, 0, "%.100g", v);
  // If the decimal size is 10 or less, use it.
  if(sz > 10)
  {
    // How many characters required in the hex string?
    const int hsz = std::snprintf(nullptr, 0, "%a", v);
    // If the hex size is less than the decimal size, use it.
    if(hsz < sz)
    {
      sz = hsz;
      useh = true;
    }
  }
  // Note +1 for null terminator since the returned size doesn't include it.
  std::vector<char> buf(sz + 1);
  if(useh)
    std::snprintf(&buf[0], buf.size(), "%a", v);
  else
    std::snprintf(&buf[0], buf.size(), "%.100g", v);

  // NOTE: LC_GLOBAL_HANDLE? Seems to be a mistake in the uselocale() documentation example.
  //uselocale(LC_GLOBAL_HANDLE);
  // So the new locale is no longer in use.
  uselocale(LC_GLOBAL_LOCALE);
  freelocale(nloc);

  return QString(&buf[0]);
}

double museStringToDouble(const QString &s, bool *ok)
{
  const char *sc = s.toLatin1().constData();
  char *end;

  // NOTE: strtod is locale sensitive! We must force the locale to standard 'C'.
  // Use a per-thread thread-safe technique instead of setting the global locale.
  // Note this C locale is NOT the same as the application's C++ locale.
  // Setting LANG environment variable before running is different than setting our -l switch.
  locale_t nloc = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
  assert(nloc != (locale_t) 0);
  uselocale(nloc);

  const double rv = std::strtod(sc, &end);
  if(ok)
    // If the conversion fails, strtod sets end = input string.
    *ok = end != sc;

  // NOTE: LC_GLOBAL_HANDLE? Seems to be a mistake in the uselocale() documentation example.
  //uselocale(LC_GLOBAL_HANDLE);
  // So the new locale is no longer in use.
  uselocale(LC_GLOBAL_LOCALE);
  freelocale(nloc);

  return rv;
}

} // namespace MusELib

