//=========================================================
//  MusE
//  Linux Music Editor
//
//  type_defs.h
//  Copyright (C) 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __TYPE_DEFS_H__
#define __TYPE_DEFS_H__

#include "stdint.h"

namespace MusECore {

// Song changed flags:
// These are flags, usually passed by connecting to the songChanged() signal,
//  which inform that various things have changed and appropriate action should 
//  be taken (redraw, refill lists etc.) upon the signal's reception.
// NOTE: Use the SongChangedStruct_t typedef to support all the bits.

#define SC_TRACK_INSERTED             1
#define SC_TRACK_REMOVED              2
#define SC_TRACK_MODIFIED             4
#define SC_PART_INSERTED              8
#define SC_PART_REMOVED               0x10
#define SC_PART_MODIFIED              0x20
#define SC_EVENT_INSERTED             0x40
#define SC_EVENT_REMOVED              0x80
#define SC_EVENT_MODIFIED             0x100
#define SC_SIG                        0x200        // timing signature
#define SC_TEMPO                      0x400        // tempo map changed
#define SC_MASTER                     0x800        // master flag changed
#define SC_SELECTION                  0x1000       // event selection. part and track selection have their own.
#define SC_MUTE                       0x2000       // A track's mute or off state changed.
#define SC_SOLO                       0x4000
#define SC_RECFLAG                    0x8000
#define SC_ROUTE                      0x10000      // A route was added, changed, or deleted. Or a midi track's out channel/port was changed.
#define SC_CHANNELS                   0x20000
#define SC_CONFIG                     0x40000      // midiPort-midiDevice
#define SC_DRUMMAP                    0x80000     // must update drumeditor
#define SC_MIDI_INSTRUMENT            0x100000     // A midi port or device's instrument has changed
#define SC_AUDIO_CONTROLLER           0x200000     // An audio controller value was added deleted or modified.
#define SC_AUTOMATION                 0x400000     // A track's automation mode setting changed (off, read, touch, write etc).
#define SC_AUX                        0x800000    // A mixer aux was added or deleted. Not adjusted.
#define SC_RACK                       0x1000000    // mixer rack changed
#define SC_CLIP_MODIFIED              0x2000000
#define SC_MIDI_CONTROLLER_ADD        0x4000000    // a hardware midi controller was added or deleted
// SC_MIDI_TRACK_PROP: A midi track's properties changed (name, thru etc). 
// For fairly 'static' properties, not frequently changing transp del compr velo or len, 
//  nor output channel/port (use SC_ROUTE).
#define SC_MIDI_TRACK_PROP            0x8000000   
#define SC_PART_SELECTION             0x10000000   // part selection changed
#define SC_KEY                        0x20000000   // key map changed
#define SC_TRACK_SELECTION            0x40000000   // track selection changed
#define SC_PORT_ALIAS_PREFERENCE      0x80000000  // (Jack) port alias viewing preference has changed
#define SC_ROUTER_CHANNEL_GROUPING    0x100000000  // Router channel grouping changed
#define SC_AUDIO_CONTROLLER_LIST      0x200000000  // An audio controller list was added deleted or modified.
#define SC_PIANO_SELECTION            0x400000000  // Piano keyboard selected note changed.
#define SC_DRUM_SELECTION             0x800000000  // Drum list selected note changed.
#define SC_TRACK_REC_MONITOR          0x1000000000 // Audio or midi track's record monitor changed.
#define SC_TRACK_MOVED                0x2000000000 // Audio or midi track's position in track list or mixer changed.
#define SC_TRACK_RESIZED              0x4000000000 // Audio or midi track was resized in the arranger.
#define SC_EVERYTHING                 -1           // global update
  
typedef int64_t SongChangedFlags_t;
typedef int64_t SongChangedSubFlags_t;

struct SongChangedStruct_t
{
  // Combination of SC_XX flags.
  SongChangedFlags_t _flags;
  // Additional optional flags.
  SongChangedSubFlags_t _subFlags;
  // An optional pointer to the object which initiated this song change.
  // The object's own songChanged() slot (if present) can use this to
  //  ignore self-generated songChanged signals. This is the only practical
  //  mechanism available for objects needing to do so. There's really
  //  no other easy way to igore such signals.
  void* _sender;
  
  SongChangedStruct_t(SongChangedFlags_t flags = 0, SongChangedSubFlags_t subFlags = 0, void* sender = 0) :
    _flags(flags), _subFlags(subFlags), _sender(sender) { };
    
  SongChangedStruct_t& operator|=(const SongChangedStruct_t& f)
  { _flags |= f._flags; _subFlags |= f._subFlags; return *this; }
    
  SongChangedStruct_t& operator|=(SongChangedFlags_t flags)
  { _flags |= flags; return *this; }
    
  SongChangedStruct_t& operator&=(const SongChangedStruct_t& f)
  { _flags &= f._flags; _subFlags &= f._subFlags; return *this; }
    
  SongChangedStruct_t& operator&=(SongChangedFlags_t flags)
  { _flags &= flags; return *this; }
    
//   friend SongChangedStruct_t operator|(const SongChangedStruct_t& a, const SongChangedStruct_t& b)
//   { SongChangedStruct_t c = a; return c |= b; }
  friend SongChangedStruct_t operator|(SongChangedStruct_t a, const SongChangedStruct_t& b)
  { return a |= b; }
   
//   friend SongChangedStruct_t operator|(const SongChangedStruct_t& a, SongChangedFlags_t b)
//   { SongChangedStruct_t c = a; return c |= b; }
//   friend SongChangedStruct_t operator|(SongChangedStruct_t a, SongChangedFlags_t b)
//   { return a |= b; }
  
  friend SongChangedFlags_t operator|(const SongChangedStruct_t& a, SongChangedFlags_t b)
  { return a._flags | b; }
  
//   friend SongChangedStruct_t operator&(const SongChangedStruct_t& a, const SongChangedStruct_t& b)
//   { SongChangedStruct_t c = a; return c &= b; }
  friend SongChangedStruct_t operator&(SongChangedStruct_t a, const SongChangedStruct_t& b)
  { return a &= b; }
   
// //   friend SongChangedStruct_t operator&(const SongChangedStruct_t& a, SongChangedFlags_t b)
// //   { SongChangedStruct_t c = a; return c &= b; }
// //   friend SongChangedStruct_t operator&(SongChangedStruct_t a, SongChangedFlags_t b)
// //   { return a &= b; }
  
//   friend SongChangedFlags_t operator&(const SongChangedStruct_t& a, SongChangedFlags_t b)
//   { return a._flags & b; }
};

typedef int64_t EventID_t;
#define MUSE_INVALID_EVENT_ID   -1

}   // namespace MusECore

#endif
