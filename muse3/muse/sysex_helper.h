//=========================================================
//  MusE
//  Linux Music Editor
//
//  sysex_helper.h
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

#ifndef __SYSEX_HELPER_H__
#define __SYSEX_HELPER_H__

namespace MusECore {

// Expected duration in frames, at the current sample rate, of the 
//  given length of sysex data. Based on 31250Hz midi baud rate in
//  1-8-2 format. (Midi specs say 1 stop bit, but ALSA says
//  2 stop bits are common.) A small gap time is added as well.
// If the data includes any start/end bytes, len should also include them.
extern unsigned int sysexDuration(unsigned int len, int sampleRate);

} // namespace MusECore

#endif
