//=========================================================
//  MusE
//  Linux Music Editor
//  
//    muse_math.h
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __MUSE_MATH_H__
#define __MUSE_MATH_H__

#include <cmath>
#include "config.h"

// FIXME See top level cmake. On MingW it said M_PI exists but here M_PI was not found!
// So just look for the define for now...
// #ifndef HAVE_M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 M_PI/2
#endif

#ifndef M_PI_4
#define M_PI_4 M_PI/4
#endif

#ifndef M_1_PI
#define M_1_PI 1/M_PI
#endif

#ifndef M_2_PI
#define M_2_PI 2/M_PI
#endif

#ifndef HAVE_EXP10
#define exp10(x) (pow(10.0, (x)))
#endif

#ifndef HAVE_EXP10F
#define exp10f(x) (powf(10.0f, (x)))
#endif

#ifndef HAVE_EXP10L
#define exp10l(x) (powl(10.0l, (x)))
#endif

// Convert value to log10, rounding to nearest .000001 due to inaccuracy with log.
#define muse_log10r(x) (round(log10(x) * 1000000.0) * 0.000001)
// Convert value to dB, rounding to nearest .000001 due to inaccuracy with log.
#define muse_val2dbr(x) (round(log10(x) * 20000000.0) * 0.000001)
// Convert value to dB.
#define muse_val2db(x) (log10(x) * 20.0)
// Convert dB to val.
#define muse_db2val(x) (pow(10, ((x) * 0.05)))
// Round to the nearest .000001
#define muse_round2micro(x) (round((x) * 1000000.0) * 0.000001)

inline double museValToDb(double v, double scale = 20.0) { return log10(v) * scale; }
inline double museDbToVal(double v, double scale = 0.05) { return exp10(v * scale); }
inline double museMin(double a, double b) { return fmin(a, b); }
inline double museMax(double a, double b) { return fmax(a, b); }

// Returns a suggested range minimum value based on combinations of type linear, log, integer, or dB.
// For log type, when minimum <= 0.0 this returns a dynamic suggested minimum based on the maximum.
// For log + dB type, when minimum <= 0.0 this returns exp10(minDB * dBFactor).
// For log + integer + dB type, when minimum <= 0.0 this returns max(1, int(maximum * exp10(minDB * dBFactor))).
// For all other cases, minimum is returned unmodified.
// Note that dBScale is an inversion of a dB factor which is usually 20.0 by default.
inline double museRangeMinValHint(
  const double &minimum, const double &maximum,
  const bool &isLog, const bool &isInt, const bool &isDB,
  const double &minDB = -120.0, const double &dBScale = 0.05)
{
  // If minimum is already NOT zero, just return it.
  if(minimum > 0.0)
    return minimum;

  if(isLog)
  {
    if(isInt)
    {
      if(isDB)
      {
        // Special for when the minimum is 0.0 (-inf dB):
        // Force the minimum to the app's minimum slider dB setting, in an integer form.
        // What's the minimum slider dB value in log form, in terms of integers?
        int mn_sldr_log_int = int(maximum * museDbToVal(minDB, dBScale));
        if(mn_sldr_log_int <= 0)
            mn_sldr_log_int = 1;
        // Check if we need to increment to the next higher integer value.
        //if(museValToDb((double(mn_sldr_log_int) / upper), 1.0 / dBScale) < minSliderDB)
        //  ++mn_sldr_log_int;
        return double(mn_sldr_log_int);
      }
      // Prefer int not dB.
      else
      {
        return minimum;
      }
    }
    // Log and not int.
    else
    {
      // Log and dB display.
      if(isDB)
      {
        // Special for when the minimum is 0.0 (-inf dB):
        // Force the slider and label minimum to the app's minimum slider dB setting.
        return museDbToVal(minDB, dBScale);
      }
      // Log and not dB display.
      else
      {
        // Special for when the minimum is 0.0:
        // Force the minimum to an appropriate non-zero value.
        // TODO: Hm, we need a minimum but our minimum dB settings may not be appropriate here.
        // We could let the controls' built in minimum (usually -120 dB) handle it, but that
        //  also may not be appropriate here.
        //min = museDbToVal(minSliderDB, dBFactor);
        //min = 0.000000001; // 1 billionth.
        if(maximum >= 10000.0)
          return 0.1;
        else if(maximum >= 100.0)
          return 0.01;
        else if(maximum >= 1.0)
          return 0.001;
        else if(maximum >= 0.01)
          return 0.0001;
        else if(maximum >= 0.0001)
          return 0.000001;
        else
          return 0.000000001;
      }
    }
  }
  // Int or none, and not log.
  else
  {
    return minimum;
  }
}

#endif

