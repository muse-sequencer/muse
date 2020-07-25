//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midi.h,v 1.4.2.2 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MIDI_H__
#define __MIDI_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "midi_consts.h"

class QString;

namespace MusECore {

class EventList;

enum AudioTickSound {
    beatSound,
    measureSound,
    accent1Sound,
    accent2Sound
};

class MidiInstrument;
extern QString nameSysex(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
extern QString sysexComment(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
extern QString midiMetaName(int meta);

class MPEventList;
class MidiTrack;
// Division can be zero meaning the event times are to be taken verbosely
//  (as ticks already), no conversion is to be applied.
extern void buildMidiEventList(EventList* mel, const MPEventList& el, MidiTrack* track, int division, bool addSysexMeta, bool doLoops);

} // namespace MusECore

#endif

