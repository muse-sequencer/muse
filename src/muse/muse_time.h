//=========================================================
//  MusE
//  Linux Music Editor
//
//  muse_time.h
//  Copyright (C) 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MUSE_TIME_H__
#define __MUSE_TIME_H__

#include <stdint.h>

namespace MusECore {

typedef int64_t MuseFrame_t;
typedef int64_t MuseCount_t;
typedef int64_t MuseTick_t;

// Handy casting define to go from our current (32-bit) unsigned int to 64-bit integer time values.
// To preserve the upper negative bits, we first cast as int then as int64.
// This define could be changed later to just (x) if the entire program switches to int64 for all time values.
#define MUSE_TIME_UINT_TO_INT64 (int64_t)(int)

}   // namespace MusECore

#endif
