//=========================================================
//  MusE
//  Linux Music Editor
//
//  sysex_helper.cpp
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "sysex_helper.h"

namespace MusECore {

//---------------------------------------------------------
//    sysexDuration
//---------------------------------------------------------

unsigned int sysexDuration(unsigned int len, int sampleRate)
{
  // Midi transmission characters per second, based on standard fixed bit rate of 31250 Hz.
  // According to ALSA (aplaymidi.c), although the midi standard says one stop bit,
  //  two are commonly used. We will use two just to be sure.
  const unsigned int midi_cps = 31250 / (1 + 8 + 2);
  // Estimate the number of audio frames it should take (or took) to transmit the current midi chunk.
  unsigned int frames = (len * sampleRate) / midi_cps;
  // Add a slight delay between chunks just to be sure there's no overlap, rather a small space, and let devices catch up.
  frames += sampleRate / 200; // 1 / 200 = 5 milliseconds.
  // Let's be realistic, spread by at least one frame.
  if(frames == 0)
    frames = 1;
  return frames;
}
  
} // namespace MusECore
