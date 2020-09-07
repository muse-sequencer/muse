//=========================================================
//  MusE
//  Linux Music Editor
//  midi_consts.h
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

#ifndef __MIDI_CONSTS_H__
#define __MIDI_CONSTS_H__

namespace MusECore {

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
      ME_SENSE       = 0xfe,
      ME_RESET       = 0xff
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
      ME_META_TEXT_8_PROGRAM_NAME = 0x08,
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
    accent2Sound
};

const unsigned char gmOnMsg[]   = { 0x7e, 0x7f, 0x09, 0x01 };
const unsigned char gm2OnMsg[]  = { 0x7e, 0x7f, 0x09, 0x03 };
const unsigned char gmOffMsg[]  = { 0x7e, 0x7f, 0x09, 0x02 };
const unsigned char gsOnMsg[]   = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41 };
const unsigned char gsOnMsg2[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x33, 0x50, 0x3c };
const unsigned char gsOnMsg3[]  = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x34, 0x50, 0x3b };
const unsigned char xgOnMsg[]   = { 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00 };
const unsigned int  gmOnMsgLen  = sizeof(gmOnMsg);
const unsigned int  gm2OnMsgLen = sizeof(gm2OnMsg);
const unsigned int  gmOffMsgLen = sizeof(gmOffMsg);
const unsigned int  gsOnMsgLen  = sizeof(gsOnMsg);
const unsigned int  gsOnMsg2Len = sizeof(gsOnMsg2);
const unsigned int  gsOnMsg3Len = sizeof(gsOnMsg3);
const unsigned int  xgOnMsgLen  = sizeof(xgOnMsg);

const unsigned char mmcDeferredPlayMsg[] = { 0x7f, 0x7f, 0x06, 0x03 };
const unsigned char mmcStopMsg[] =         { 0x7f, 0x7f, 0x06, 0x01 };
const unsigned char mmcLocateMsg[] =       { 0x7f, 0x7f, 0x06, 0x44, 0x06, 0x01, 0, 0, 0, 0, 0 };

const unsigned int  mmcDeferredPlayMsgLen = sizeof(mmcDeferredPlayMsg);
const unsigned int  mmcStopMsgLen = sizeof(mmcStopMsg);
const unsigned int  mmcLocateMsgLen = sizeof(mmcLocateMsg);

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

} // namespace MusECore

#endif


