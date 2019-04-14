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

#ifndef HAVE_EXP10
#define exp10(x) (pow(10.0, x))
#endif

#ifndef HAVE_EXP10F
#define exp10f(x) (powf(10.0f, x))
#endif

#ifndef HAVE_EXP10L
#define exp10l(x) (powl(10.0l, x))
#endif

// Convert value to log10, rounding to nearest .000001 due to inaccuracy with log.
#define muse_log10r(x) (round(log10(x) * 1000000.0) * 0.000001)
// Convert value to dB, rounding to nearest .000001 due to inaccuracy with log.
#define muse_val2dbr(x) (round(log10(x) * 20000000.0) * 0.000001)
// Convert dB to val.
#define muse_db2val(x) pow(10, (x * 0.05))
// Round to the nearest .000001
#define muse_round2micro(x) (round(x * 1000000.0) * 0.000001)

#endif

