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
extern QString nameSysex(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
extern QString sysexComment(unsigned int len, const unsigned char* buf, MidiInstrument* instr = 0);
extern QString midiMetaName(int meta);
// Expected duration in frames, at the current sample rate, of the 
//  given length of sysex data. Based on 31250Hz midi baud rate in
//  1-8-2 format. (Midi specs say 1 stop bit, but ALSA says
//  2 stop bits are common.) A small gap time is added as well.
// If the data includes any start/end bytes, len should also include them.
extern unsigned int sysexDuration(unsigned int len);

// Use these in all the synths and their guis.
// NOTE: Some synths and hosts might not use this scheme. For example, MESS requires it for IPC,
//        and both MESS and DSSI use it to store init data in the form of a sysex.
//
// Added this here for ease, since they all include this file.
//
// A special MusE soft synth sysex manufacturer ID.
#define MUSE_SYNTH_SYSEX_MFG_ID 0x7c
// Following the MFG_ID, besides synth specific IDs, this reserved special ID indicates
//  a MusE SYSTEM ID will follow in the next byte.
#define MUSE_SYSEX_SYSTEM_ID 0x7f
// This SYSTEM command will force any relevant drum maps to update.
// When a synth's note names have changed, it should issue this command.
// So far, this command is really just a special requirement for the fluidsynth MESS plugin.
// It is the only way to inform the host to update the maps.
#define MUSE_SYSEX_SYSTEM_UPDATE_DRUM_MAPS_ID 0x00

// REMOVE Tim. autoconnect. Added.
// // This SYSTEM command will send CTRL_ALL_SOUNDS_OFF and/or CTRL_RESET_ALL_CTRL,
// //  where appropriate reset all MidiCtrlValList lastValidByteX members,
// //  and reset all MidiDevice curOutParamNums members. It is for internal use. It is sent
// //  to the drivers like any other message (played or putEvent etc.), but it is not transmitted.
// // It's the only way to tell our ring-buffered drivers to reset their cached values to CTRL_VAL_UNKNOWN.
// #define MUSE_SYSEX_SYSTEM_PANIC_ID 0x01
// // The next byte describes what to include in the panic. It is an OR'd combination of these values:
// enum MUSE_SYSEX_SYSTEM_PANIC_TYPES { MUSE_SYSEX_SYSTEM_PANIC_ALL_SOUNDS_OFF = 0x1, MUSE_SYSEX_SYSTEM_PANIC_RESET_ALL_CTRL = 0x2};

class MPEventList;
class MidiTrack;
extern void buildMidiEventList(EventList* mel, const MPEventList& el, MidiTrack* track, int division, bool addSysexMeta, bool doLoops);

} // namespace MusECore

#endif

