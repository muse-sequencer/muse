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

#include <stdint.h>

namespace MusECore {

// REMOVE Tim. Added. Moved here from event.h.
// NOTICE: The values 3 and 4 (PAfter and CAfter) are reserved for the support of those two obsolete
//          channel and key pressure events in old files. They are converted to controllers upon load.
enum EventType { Note=0, Controller=1, Sysex=2, /*PAfter=3,*/ /*CAfter=4,*/ Meta=5, Wave=6 };
  
typedef int64_t SongChangedFlags_t;

struct SongChangedStruct_t
{
  private:
  //
  // 128-bit combination of 128-bit SC_XX flags:
  //
  // Normal lower 64 bits of 128-bit flags.
  SongChangedFlags_t _flagsLo;
  // Upper 64 bits of 128-bit flags.
  SongChangedFlags_t _flagsHi;

  public:
  // An optional pointer to the object which initiated this song change.
  // The object's own songChanged() slot (if present) can use this to
  //  ignore self-generated songChanged signals. This is the only practical
  //  mechanism available for objects needing to do so. There's really
  //  no other easy way to ignore such signals.
  void* _sender;

  public:
  inline SongChangedStruct_t(SongChangedFlags_t flagsLo = 0, SongChangedFlags_t flagsHi = 0, void* sender = 0) :
    _flagsLo(flagsLo), _flagsHi(flagsHi), _sender(sender) { };

  SongChangedFlags_t flagsLo() const { return _flagsLo; }
  SongChangedFlags_t flagsHi() const { return _flagsHi; }
    
  // C++11: Avoid necessity of using the "Safe Bool Idiom".
  explicit inline operator bool() const { return _flagsLo || _flagsHi; }

  inline SongChangedStruct_t operator~() const
  { SongChangedStruct_t r(~_flagsLo, ~_flagsHi); return r; }

  inline bool operator==(const SongChangedStruct_t& f) const { return _flagsLo == f._flagsLo && _flagsHi == f._flagsHi; }
  inline bool operator!=(const SongChangedStruct_t& f) const { return _flagsLo != f._flagsLo || _flagsHi != f._flagsHi; }
  
  inline SongChangedStruct_t& operator|=(const SongChangedStruct_t& f)
  { _flagsLo |= f._flagsLo; _flagsHi |= f._flagsHi; return *this; }

  inline SongChangedStruct_t& operator&=(const SongChangedStruct_t& f)
  { _flagsLo &= f._flagsLo; _flagsHi &= f._flagsHi; return *this; }


  inline friend SongChangedStruct_t operator|(const SongChangedStruct_t& a, const SongChangedStruct_t& b)
  { SongChangedStruct_t r(a); r |= b; return r; }

  inline friend SongChangedStruct_t operator&(const SongChangedStruct_t& a, const SongChangedStruct_t& b)
  { SongChangedStruct_t r(a); r &= b; return r; }
};

// Song changed flags:
// These are flags, usually passed by connecting to the songChanged() signal,
//  which inform that various things have changed and appropriate action should 
//  be taken (redraw, refill lists etc.) upon the signal's reception.
// NOTE: Use the SongChangedStruct_t typedef to support all the bits.

#define SC_NOTHING                    MusECore::SongChangedStruct_t(0, 0)
#define SC_TRACK_INSERTED             MusECore::SongChangedStruct_t(1)
#define SC_TRACK_REMOVED              MusECore::SongChangedStruct_t(2)
#define SC_TRACK_MODIFIED             MusECore::SongChangedStruct_t(4)
#define SC_PART_INSERTED              MusECore::SongChangedStruct_t(8)
#define SC_PART_REMOVED               MusECore::SongChangedStruct_t(0x10)
#define SC_PART_MODIFIED              MusECore::SongChangedStruct_t(0x20)
#define SC_EVENT_INSERTED             MusECore::SongChangedStruct_t(0x40)
#define SC_EVENT_REMOVED              MusECore::SongChangedStruct_t(0x80)
#define SC_EVENT_MODIFIED             MusECore::SongChangedStruct_t(0x100)
#define SC_SIG                        MusECore::SongChangedStruct_t(0x200)        // timing signature
#define SC_TEMPO                      MusECore::SongChangedStruct_t(0x400)        // tempo map changed
#define SC_MASTER                     MusECore::SongChangedStruct_t(0x800)        // master flag changed
#define SC_SELECTION                  MusECore::SongChangedStruct_t(0x1000)       // event selection. part and track selection have their own.
#define SC_MUTE                       MusECore::SongChangedStruct_t(0x2000)       // A track's mute or off state changed.
#define SC_SOLO                       MusECore::SongChangedStruct_t(0x4000)
#define SC_RECFLAG                    MusECore::SongChangedStruct_t(0x8000)
#define SC_ROUTE         MusECore::SongChangedStruct_t(0x10000)  // A route was added, changed, or deleted. Or a midi track's out channel/port was changed.
#define SC_CHANNELS                   MusECore::SongChangedStruct_t(0x20000)
#define SC_CONFIG                     MusECore::SongChangedStruct_t(0x40000)      // midiPort-midiDevice
#define SC_DRUMMAP                    MusECore::SongChangedStruct_t(0x80000)     // must update drumeditor
#define SC_MIDI_INSTRUMENT            MusECore::SongChangedStruct_t(0x100000)     // A midi port or device's instrument has changed
#define SC_AUDIO_CONTROLLER           MusECore::SongChangedStruct_t(0x200000)     // An audio controller value was added deleted or modified.
#define SC_AUTOMATION                 MusECore::SongChangedStruct_t(0x400000)     // A track's automation mode setting changed (off, read, touch, write etc).
#define SC_AUX                        MusECore::SongChangedStruct_t(0x800000)    // A mixer aux was added or deleted. Not adjusted.
#define SC_RACK                       MusECore::SongChangedStruct_t(0x1000000)    // mixer rack changed
#define SC_CLIP_MODIFIED              MusECore::SongChangedStruct_t(0x2000000)
#define SC_MIDI_CONTROLLER_ADD        MusECore::SongChangedStruct_t(0x4000000)    // a hardware midi controller was added or deleted
// SC_MIDI_TRACK_PROP: A midi track's properties changed (name, thru etc). 
// For fairly 'static' properties, not frequently changing transp del compr velo or len, 
//  nor output channel/port (use SC_ROUTE).
#define SC_MIDI_TRACK_PROP            MusECore::SongChangedStruct_t(0x8000000)
#define SC_PART_SELECTION             MusECore::SongChangedStruct_t(0x10000000)   // part selection changed
#define SC_KEY                        MusECore::SongChangedStruct_t(0x20000000)   // key map changed
#define SC_TRACK_SELECTION            MusECore::SongChangedStruct_t(0x40000000)   // track selection changed
#define SC_PORT_ALIAS_PREFERENCE      MusECore::SongChangedStruct_t(0x80000000)  // (Jack) port alias viewing preference has changed
#define SC_ROUTER_CHANNEL_GROUPING    MusECore::SongChangedStruct_t(0x100000000)  // Router channel grouping changed
#define SC_AUDIO_CONTROLLER_LIST      MusECore::SongChangedStruct_t(0x200000000)  // An audio controller list was added deleted or modified.
#define SC_PIANO_SELECTION            MusECore::SongChangedStruct_t(0x400000000)  // Piano keyboard selected note changed.
#define SC_DRUM_SELECTION             MusECore::SongChangedStruct_t(0x800000000)  // Drum list selected note changed.
#define SC_TRACK_REC_MONITOR          MusECore::SongChangedStruct_t(0x1000000000) // Audio or midi track's record monitor changed.
#define SC_TRACK_MOVED                MusECore::SongChangedStruct_t(0x2000000000) // Audio or midi track's position in track list or mixer changed.
#define SC_TRACK_RESIZED              MusECore::SongChangedStruct_t(0x4000000000) // Audio or midi track was resized in the arranger.
#define SC_METRONOME                  MusECore::SongChangedStruct_t(0x8000000000) // Metronome lists settings such as accents changed.
#define SC_EXTERNAL_MIDI_SYNC         MusECore::SongChangedStruct_t(0x10000000000) // External midi sync flag changed.
#define SC_USE_JACK_TRANSPORT         MusECore::SongChangedStruct_t(0x20000000000) // UseJackTransport flag changed.
#define SC_TIMEBASE_MASTER            MusECore::SongChangedStruct_t(0x40000000000) // Timebase master state changed.
#define SC_AUDIO_CONVERTER            MusECore::SongChangedStruct_t(0x80000000000) // Audio converters settings or value lists changed.
#define SC_AUDIO_STRETCH              MusECore::SongChangedStruct_t(0x100000000000) // Audio converters stretch/pitch ratios changed.
#define SC_MARKER_INSERTED            MusECore::SongChangedStruct_t(0x200000000000)
#define SC_MARKER_REMOVED             MusECore::SongChangedStruct_t(0x400000000000)
#define SC_MARKER_MODIFIED            MusECore::SongChangedStruct_t(0x800000000000)
// The marker list was rebuilt as a result of tempo changes. NOTE: Currently signals/slots are used for add/remove/modify etc.
#define SC_MARKERS_REBUILT            MusECore::SongChangedStruct_t(0x1000000000000)
// The midi division changed. Re-normalization of tempo and signature lists will have already occurred.
#define SC_DIVISION_CHANGED           MusECore::SongChangedStruct_t(0x2000000000000)
#define SC_EVERYTHING                 MusECore::SongChangedStruct_t(-1, -1)       // global update


typedef int64_t EventID_t;
#define MUSE_INVALID_EVENT_ID   -1
#define MUSE_INVALID_POSITION   INT_MAX

enum class ResizeDirection {
      RESIZE_TO_THE_LEFT,
      RESIZE_TO_THE_RIGHT
};

enum RelevantSelectedEvents { NoEventsRelevant = 0x00, NotesRelevant = 0x01, ControllersRelevant = 0x02,
                SysexRelevant = 0x04, MetaRelevant = 0x08, WaveRelevant = 0x10,
                AllEventsRelevant = NotesRelevant | ControllersRelevant |
                                    SysexRelevant | MetaRelevant | WaveRelevant};
typedef int RelevantSelectedEvents_t;

enum FunctionOptions {
  FunctionNoOptions = 0x00,
  // For pasting. Whether to cut the given items before pasting.
  // Don't call cut_items() AND then set this flag on paste_at().
  // Here, cutting is usually reserved for direct pasting
  //  (calling paste_items_at() with an EventTagList*).
  FunctionCutItems = 0x01,
  // Always paste into a new part.
  FunctionPasteAlwaysNewPart = 0x02,
  // Never paste into a new part.
  FunctionPasteNeverNewPart = 0x04,
  // Erase existing target controller items first.
  FunctionEraseItems = 0x08,
  // If FunctionEraseItems is set: How to handle the last item in any 'cluster' of controller events.
  FunctionEraseItemsWysiwyg = 0x10,
  // If FunctionEraseItems is set: Erase existing target items in empty source space between 'clusters'.
  FunctionEraseItemsInclusive = 0x20,
  FunctionEraseItemsDefault =
    FunctionEraseItems | FunctionEraseItemsWysiwyg,
  FunctionAllOptions =
    FunctionCutItems | FunctionPasteAlwaysNewPart | FunctionPasteNeverNewPart |
    FunctionEraseItems | FunctionEraseItemsWysiwyg | FunctionEraseItemsInclusive
};
typedef int FunctionOptions_t;

struct FunctionOptionsStruct
{
  FunctionOptions_t _flags;
  
  FunctionOptionsStruct(const FunctionOptions_t& flags = FunctionEraseItemsDefault) : _flags(flags) { }
  void clear() { _flags = FunctionNoOptions; }
  void setFlags(const FunctionOptions_t& flags) { _flags = flags; }
  void appendFlags(const FunctionOptions_t& flags) { _flags |= flags; }
  void removeFlags(const FunctionOptions_t& flags) { _flags &= ~flags; }
};


}   // namespace MusECore

#endif
