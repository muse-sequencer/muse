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

// #include <sys/time.h>
#include <stdint.h>

namespace MusECore {

// // REMOVE Tim. timing. Added.
// typedef uint64_t MuseTime_t; // In microseconds.
// //typedef timeval MuseTime_t;
// //typedef uint32_t MuseFrames_t;
//   
// // class MuseTime
// // {
// //   public:
// //     MuseTime();
// // };
// 
// inline museTimeMulS(MuseTime_t /*tv*/, int /*val*/)
// {
//     
// }
  
  
//typedef double MuseFloatTick;

// REMOVE Tim. samplerate. Added.
typedef int64_t MuseFrame_t;
typedef int64_t MuseCount_t;


}   // namespace MusECore

#endif
