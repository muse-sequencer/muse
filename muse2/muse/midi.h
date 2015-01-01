//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midi.h,v 1.4.2.2 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

class QString;

namespace MusECore {

class EventList;

enum {
      ME_NOTEOFF     = 0x80,
      ME_NOTEON      = 0x90,
      ME_POLYAFTER   = 0xa0,
      ME_CONTROLLER  = 0xb0,
      ME_PROGRAM     = 0xc0,
      ME_AFTERTOUCH  = 0xd0,
      ME_PITCHBEND   = 0xe0,
      ME_SYSEX       = 0xf0,
      ME_META        = 0xff,
      ME_MTC_QUARTER = 0xf1,
      ME_SONGPOS     = 0xf2,
      ME_SONGSEL     = 0xf3,
      ME_TUNE_REQ    = 0xf6,
      ME_SYSEX_END   = 0xf7,
      ME_CLOCK       = 0xf8,
      ME_TICK        = 0xf9,
      ME_START       = 0xfa,
      ME_CONTINUE    = 0xfb,
      ME_STOP        = 0xfc,
      ME_SENSE       = 0xfe
      };

//--------------------------------------      
// Recognized / transmitted  meta events:
//--------------------------------------      
enum {
      ME_META_TEXT_0_SEQUENCE_NUMBER = 0x00,
      ME_META_TEXT_1_COMMENT = 0x01,
      ME_META_TEXT_2_COPYRIGHT = 0x02,
      ME_META_TEXT_3_TRACK_NAME = 0x03,
      ME_META_TEXT_4_INSTRUMENT_NAME = 0x04,
      ME_META_TEXT_5_LYRIC = 0x05,
      ME_META_TEXT_6_MARKER = 0x06,
      ME_META_TEXT_7_CUE_POINT = 0x07,
      ME_META_TEXT_8 = 0x08,
      ME_META_TEXT_9_DEVICE_NAME = 0x09,
      ME_META_TEXT_A = 0x0a,
      ME_META_TEXT_B = 0x0b,
      ME_META_TEXT_C = 0x0c,
      ME_META_TEXT_D = 0x0d,
      ME_META_TEXT_E = 0x0e,
      ME_META_TEXT_F_TRACK_COMMENT = 0x0f,
      ME_META_CHANNEL_CHANGE = 0x20,
      ME_META_PORT_CHANGE = 0x21,
      ME_META_END_OF_TRACK = 0x2f,
      ME_META_SET_TEMPO = 0x51,
      ME_META_SMPTE_OFFSET = 0x54,
      ME_META_TIME_SIGNATURE = 0x58,
      ME_META_KEY_SIGNATURE = 0x59,
      ME_META_SEQ_SPECIFIC_1 = 0x74,
      ME_META_SEQ_SPECIFIC_2 = 0x7f
};

enum AudioTickSound {
    beatSound,
    measureSound,
    accent1Sound,
    accent2Sound,
    reloadClickSounds
};

extern const unsigned char gmOnMsg[];
extern const unsigned char gm2OnMsg[];
extern const unsigned char gmOffMsg[];

extern const unsigned char gsOnMsg[];
extern const unsigned char gsOnMsg2[];
extern const unsigned char gsOnMsg3[];
extern const unsigned char xgOnMsg[];
extern const unsigned char mmcDeferredPlayMsg[];
extern const unsigned char mmcStopMsg[];
extern const unsigned char mmcLocateMsg[];

extern const unsigned int gmOnMsgLen;
extern const unsigned int gm2OnMsgLen;
extern const unsigned int gmOffMsgLen;
extern const unsigned int gsOnMsgLen;
extern const unsigned int gsOnMsg2Len;
extern const unsigned int gsOnMsg3Len;
extern const unsigned int xgOnMsgLen;
extern const unsigned int mmcDeferredPlayMsgLen;
extern const unsigned int mmcStopMsgLen;
extern const unsigned int mmcLocateMsgLen;

class MidiInstrument;
QString nameSysex(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
QString sysexComment(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
QString midiMetaName(int);

// Use these in all the synths and their guis.
// Did this here for ease, since they all include this file.
//
// A special MusE soft synth sysex manufacturer ID.
#define MUSE_SYNTH_SYSEX_MFG_ID 0x7c

class MPEventList;
class MidiTrack;
extern void buildMidiEventList(EventList* mel, const MPEventList& el, MidiTrack* track, int division, bool addSysexMeta, bool doLoops);

} // namespace MusECore

#endif

