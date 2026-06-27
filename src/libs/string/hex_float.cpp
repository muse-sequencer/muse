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
// #include <clocale>
#include <locale.h>   // newlocale, uselocale, freelocale, LC_NUMERIC_MASK
#include <cctype>     // isspace
#include <cstdio>   // fprintf, stderr
#include <cerrno>   // errno

// Forwards from header:
#include <QString>


namespace MusELib {


#ifndef HAVE_ISTRINGSTREAM_HEXFLOAT
// NOTE: hexfloatDecimalPoint is no longer used for decimal-point replacement.
//
// CORRECTION: an earlier comment here claimed strtod/strtof parse hex-float
// literals with '.' "regardless of system locale". That is WRONG: strtod/strtof
// ARE locale sensitive for the decimal-point character, even inside a hex-float
// significand. Under a comma-locale (e.g. de_DE) "0x1.aaaaaap-1" stops at the '.'
// and mis-parses. We therefore force the "C" locale via uselocale() in the
// cLocaleStrToNum() helpers below (see museStringToDouble/Float).
//
// This variable is kept only for ABI/symbol compatibility. The original bug it
// related to was the Qt6-incompatible QString('.') constructor (resolved to
// QString(int=46, QChar()) under Qt6), not a locale-conversion issue.
QString hexfloatDecimalPoint = QString(QChar('.'));
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


QString museStringFromFloat(float v)
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




#ifndef HAVE_ISTRINGSTREAM_HEXFLOAT
  // add some helper functions for locale handling 
  // 
  // Parse a hex-float (or any) literal under the "C" locale, regardless of the
  // process locale. strtod/strtof ARE locale sensitive for the decimal point,
  // even inside a hex-float significand: under a comma-locale (e.g. de_DE)
  // "0x1.aaaaaap-1" otherwise stops at the '.', yielding 0x1 and a parse failure.
  // uselocale() switches only the calling thread's locale, so it is thread-safe
  // (unlike setlocale()). Returns true if something was parsed and only
  // whitespace/'\0' remains after the number.
  static bool cLocaleConsumed(const char *sc, char *end)
  {
    if(end == sc)
      return false;
    while(*end)
    {
      if(!std::isspace(static_cast<unsigned char>(*end)))
        return false;
      ++end;
    }
    return true;
  }


  static bool cLocaleStrToNum(const char *sc, double &out)
  {
    if(!sc)
    {
      fprintf(stderr, "cLocaleStrToNum: null input string\n");
      out = 0;
      return false;
    }
    char *end = nullptr;
    locale_t cloc = newlocale(LC_NUMERIC_MASK, "C", static_cast<locale_t>(0));
    if(cloc != static_cast<locale_t>(0))
    {
      locale_t old = uselocale(cloc);
      out = std::strtod(sc, &end);
      uselocale(old);
      freelocale(cloc);
    }
    else
    {
      // newlocale() failed: we cannot force the "C" locale for this thread.
      // Parse anyway, but warn: under a comma-locale a hex-float significand
      // ("0x1.aaaaaap-1") will mis-parse at the '.'. Not silent on purpose.
      fprintf(stderr,
        "cLocaleStrToNum(double): newlocale(LC_NUMERIC,\"C\") failed (errno=%d), "
        "parsing '%s' in process locale - hex-floats may be wrong\n",
        errno, sc ? sc : "(null)");
      out = std::strtod(sc, &end);
    }
    return cLocaleConsumed(sc, end);
  }

  static bool cLocaleStrToNum(const char *sc, float &out)
  {
    if(!sc)
    {
      fprintf(stderr, "cLocaleStrToNum: null input string\n");
      out = 0;
      return false;
    }
    char *end = nullptr;
    locale_t cloc = newlocale(LC_NUMERIC_MASK, "C", static_cast<locale_t>(0));
    if(cloc != static_cast<locale_t>(0))
    {
      locale_t old = uselocale(cloc);
      out = std::strtof(sc, &end);
      uselocale(old);
      freelocale(cloc);
    }
    else
    {
      // See double overload: warn instead of silently parsing in the wrong locale.
      fprintf(stderr,
        "cLocaleStrToNum(float): newlocale(LC_NUMERIC,\"C\") failed (errno=%d), "
        "parsing '%s' in process locale - hex-floats may be wrong\n",
        errno, sc ? sc : "(null)");
      out = std::strtof(sc, &end);
    }
    return cLocaleConsumed(sc, end);
  }
#endif 




double museStringToDouble(const QString &s, bool *ok)
{
#ifdef HAVE_ISTRINGSTREAM_HEXFLOAT
  std::istringstream ss(s.toStdString());
  ss.imbue(std::locale("C"));
  double value = 0.0;
  ss >> value;
  if(ok)
    *ok = true;
  return value;
#else // C++ istringstream does not support hexfloat
  // Trim once up front so leading/trailing whitespace doesn't bypass the hex check.
  const QString st = s.trimmed();

  // Is it a hex number?
  if(st.startsWith("0x", Qt::CaseInsensitive) ||
     st.startsWith("-0x", Qt::CaseInsensitive) ||
     st.startsWith("+0x", Qt::CaseInsensitive))
  {
    const QByteArray ba = st.toLatin1();
    double rv = 0.0;
    const bool good = cLocaleStrToNum(ba.constData(), rv);
    if(ok)
      *ok = good;
    return rv;
  }
  else // Not a hex number.
  {
    // Plain decimal value (e.g. "0.375"). QString::toDouble() is locale-INDEPENDENT:
    // it always parses with '.' as the decimal separator (it does NOT use the
    // process/QLocale locale), so it is safe under a comma-locale like de_DE and
    // matches how museStringFromDouble() writes values in the "C" locale.
    // Therefore no uselocale("C") wrapping is needed here, unlike the strtod path
    // above where the C runtime IS locale sensitive.
    return st.toDouble(ok);
  }
#endif // HAVE_ISTRINGSTREAM_HEXFLOAT
}





float museStringToFloat(const QString &s, bool *ok)
{
#ifdef HAVE_ISTRINGSTREAM_HEXFLOAT
  std::istringstream ss(s.toStdString());
  ss.imbue(std::locale("C"));
  float value = 0.0f;
  ss >> value;
  if(ok)
    *ok = true;
  return value;
#else // C++ istringstream does not support hexfloat
  // Trim once up front so leading/trailing whitespace doesn't bypass the hex check.
  const QString st = s.trimmed();

  // Is it a hex number?
  if(st.startsWith("0x", Qt::CaseInsensitive) ||
     st.startsWith("-0x", Qt::CaseInsensitive) ||
     st.startsWith("+0x", Qt::CaseInsensitive))
  {
    const QByteArray ba = st.toLatin1();
    float rv = 0.0f;
    const bool good = cLocaleStrToNum(ba.constData(), rv);
    if(ok)
      *ok = good;
    return rv;
  }
  else // Not a hex number.
  {
    // Plain decimal value (e.g. "0.5"). QString::toFloat() is locale-INDEPENDENT:
    // it always uses '.' as the decimal separator regardless of the process/QLocale
    // locale, so it is safe under a comma-locale (de_DE) and is consistent with the
    // "C"-locale output of museStringFromFloat(). No uselocale("C") needed here.
    return st.toFloat(ok);
  }
#endif // HAVE_ISTRINGSTREAM_HEXFLOAT
}



} // namespace MusELib
