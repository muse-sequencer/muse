//=========================================================
//  MusE
//  Linux Music Editor
//  midictrl_consts.h
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MIDICTRL_CONSTS_H__
#define __MIDICTRL_CONSTS_H__

namespace MusECore {

const int CTRL_HBANK = 0x00;
const int CTRL_LBANK = 0x20;

const int CTRL_HDATA = 0x06;
const int CTRL_LDATA = 0x26;

const int CTRL_DATA_INC = 0x60;
const int CTRL_DATA_DEC = 0x61;

const int CTRL_HNRPN = 0x63;
const int CTRL_LNRPN = 0x62;

const int CTRL_HRPN  = 0x65;
const int CTRL_LRPN  = 0x64;

const int CTRL_MODULATION         = 0x01;
const int CTRL_PORTAMENTO_TIME    = 0x05;
const int CTRL_VOLUME             = 0x07;
const int CTRL_PANPOT             = 0x0a;
const int CTRL_EXPRESSION         = 0x0b;
const int CTRL_SUSTAIN            = 0x40;
const int CTRL_PORTAMENTO         = 0x41;
const int CTRL_SOSTENUTO          = 0x42;
const int CTRL_SOFT_PEDAL         = 0x43;
const int CTRL_HARMONIC_CONTENT   = 0x47;
const int CTRL_RELEASE_TIME       = 0x48;
const int CTRL_ATTACK_TIME        = 0x49;

const int CTRL_BRIGHTNESS         = 0x4a;
const int CTRL_PORTAMENTO_CONTROL = 0x54;
const int CTRL_REVERB_SEND        = 0x5b;
const int CTRL_CHORUS_SEND        = 0x5d;
const int CTRL_VARIATION_SEND     = 0x5e;

// Channel Mode controllers:
// Same as other 7-bit controllers, but "implements Mode control and
//  special message by using reserved controller numbers 120-127" (MMA).
//
const int CTRL_ALL_SOUNDS_OFF     = 0x78; // 120
const int CTRL_RESET_ALL_CTRL     = 0x79; // 121
const int CTRL_LOCAL_OFF          = 0x7a; // 122
const int CTRL_ALL_NOTES_OFF      = 0x7b; // 123
const int CTRL_OMNI_MODE_OFF      = 0x7c; // 124 All notes off as well.
const int CTRL_OMNI_MODE_ON       = 0x7d; // 125 All notes off as well.
const int CTRL_MONO_MODE_ON       = 0x7e; // 126 All notes off as well.
const int CTRL_POLY_MODE_ON       = 0x7f; // 127 All notes off as well.


// controller types 0x10000 - 0x1ffff are 14 bit controller with
//    0x1xxyy
//      xx - MSB controller
//      yy - LSB controller

// RPN  - registered parameter numbers 0x20000 -
// NRPN - non registered parameter numbers 0x30000 -

// internal controller types:
const int CTRL_INTERNAL_OFFSET = 0x40000;

const int CTRL_PITCH    = CTRL_INTERNAL_OFFSET;
const int CTRL_PROGRAM  = CTRL_INTERNAL_OFFSET      + 0x01;
const int CTRL_VELOCITY = CTRL_INTERNAL_OFFSET      + 0x02;
const int CTRL_MASTER_VOLUME = CTRL_INTERNAL_OFFSET + 0x03;
const int CTRL_AFTERTOUCH = CTRL_INTERNAL_OFFSET    + 0x04;
// NOTE: The range from CTRL_INTERNAL_OFFSET + 0x100 to CTRL_INTERNAL_OFFSET + 0x1ff is reserved 
//        for this control. (The low byte is reserved because this is a per-note control.) 
const int CTRL_POLYAFTER = CTRL_INTERNAL_OFFSET     + 0x1FF;  // 100 to 1FF !

const int CTRL_VAL_UNKNOWN   = 0x10000000; // used as unknown hwVal
const int CTRL_PROGRAM_VAL_DONT_CARE = 0xffffff; // High-bank, low-bank, and program are all 0xff don't care.

const int CTRL_7_OFFSET      = 0x00000;
const int CTRL_14_OFFSET     = 0x10000;
const int CTRL_RPN_OFFSET    = 0x20000;
const int CTRL_NRPN_OFFSET   = 0x30000;
const int CTRL_RPN14_OFFSET  = 0x50000;
const int CTRL_NRPN14_OFFSET = 0x60000;
const int CTRL_NONE_OFFSET   = 0x70000;

const int CTRL_OFFSET_MASK   = 0xf0000;

} // namespace MusECore

#endif


