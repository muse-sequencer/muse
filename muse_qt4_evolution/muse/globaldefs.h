//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

static const int MAX_CHANNELS = 2;   // max audio channels
// const int MIDI_PORTS   = 16;  // max Number of Midi Ports

#ifndef MIDI_CHANNELS
#define MIDI_CHANNELS 16       // Channels per Port

enum MidiInstrumentType {
      MT_GENERIC, MT_GM, MT_GS, MT_XG
      };

#endif

#endif

