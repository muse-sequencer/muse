//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.h,v 1.3.2.3 2009/03/09 02:05:18 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MINSTRUMENT_H__
#define __MINSTRUMENT_H__

#include "globaldefs.h"
#include <map>
#include <list>
#include <vector>
#include <string>
#include <QString>
#include "midiedit/drummap.h"

// REMOVE Tim. newdrums. Added.
// Adds the ability to override at instrument level.
// But it just makes things too complex for the user.
// And in a way is unnecessary and overkill, since we
//  already allow modifying an instrument.
//#define _USE_INSTRUMENT_OVERRIDES_

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {
class EventList;
class MidiControllerList;
class MidiPort;
class MidiPlayEvent;
class Xml;


//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch {
      signed char hbank, lbank, program;
      QString name;
      bool drum;

      inline int patch() const { return (((unsigned int)hbank & 0xff) << 16) | (((unsigned int)lbank & 0xff) << 8) | ((unsigned int)program & 0xff); }
      inline bool dontCare()        const { return hbankDontCare() && lbankDontCare() && programDontCare(); }
      inline bool hbankDontCare()   const { return hbank < 0; }
      inline bool lbankDontCare()   const { return lbank < 0; }
      inline bool programDontCare() const { return program < 0; }

      void read(Xml&);
      void write(int level, Xml&);
      };

class PatchList : public std::list<Patch*> {
  public:
    iterator find(int patch, bool drum, bool includeDefault);
    const_iterator find(int patch, bool drum, bool includeDefault) const;
};

typedef PatchList::iterator iPatch;
typedef PatchList::const_iterator ciPatch;

//---------------------------------------------------------
//   PatchGroup
//---------------------------------------------------------

struct PatchGroup {
      QString name;
      PatchList patches;
      void read(Xml&);
      };

class PatchGroupList : public std::vector<PatchGroup*> {
  public:
    Patch* findPatch(int patch, bool drum, bool includeDefault);
    Patch* findPatch(int patch, bool drum, bool includeDefault) const;
};

typedef PatchGroupList::iterator iPatchGroup;
typedef PatchGroupList::const_iterator ciPatchGroup;

struct SysEx {
      QString name;
      QString comment;
      int dataLen;
      unsigned char* data;
      bool read(Xml&);
      void write(int level, Xml&);
      
      SysEx();
      SysEx(const SysEx& src);
      ~SysEx();
      };

struct dumb_patchlist_entry_t
{
  int prog;
  int lbank;
  int hbank; // "-1" means "unused"
  
  dumb_patchlist_entry_t(int p, int l, int h)
  {
    prog=p;
    lbank=l;
    hbank=h;
  }
  
  bool operator<(const dumb_patchlist_entry_t& other) const
  {
    if (hbank < other.hbank) return true;
    if (hbank > other.hbank) return false;
    if (lbank < other.lbank) return true;
    if (lbank > other.lbank) return false;
    return (prog < other.prog);
  }
  
  bool operator>(const dumb_patchlist_entry_t& other) const
  {
    return other < *this;
  }
  
  bool operator==(const dumb_patchlist_entry_t& other) const
  {
    return (prog==other.prog && lbank==other.lbank && hbank==other.hbank);
  }
  
  bool operator!=(const dumb_patchlist_entry_t& other) const
  {
    return (!(*this==other));
  }
};

struct WorkingDrumMapEntry {
#ifdef _USE_INSTRUMENT_OVERRIDES_
  enum OverrideType { NoOverride = 0x0, TrackDefaultOverride = 0x1, TrackOverride = 0x2, InstrumentDefaultOverride = 0x4, InstrumentOverride = 0x8,
                      AllOverrides = InstrumentDefaultOverride | InstrumentOverride | TrackDefaultOverride | TrackOverride };
#else
  enum OverrideType { NoOverride = 0x0, TrackDefaultOverride = 0x1, TrackOverride = 0x2,
                      AllOverrides = TrackDefaultOverride | TrackOverride };
#endif

  enum Field { NoField = 0x0, NameField = 0x1, VolField = 0x2, QuantField = 0x4, LenField = 0x8, ChanField = 0x10, PortField = 0x20,
               Lv1Field = 0x40, Lv2Field = 0x80, Lv3Field = 0x100, Lv4Field = 0x200, ENoteField = 0x400, ANoteField = 0x800,
               MuteField = 0x1000, HideField = 0x2000,
               AllFields = NameField | VolField | QuantField | LenField | ChanField | PortField | Lv1Field | Lv2Field | Lv3Field | Lv4Field |
                           ENoteField | ANoteField | MuteField | HideField};
  // OR'd Fields.
  typedef int override_t;
  typedef int fields_t;

  DrumMap _mapItem;
  fields_t _fields;

  // Starts with null _mapItem.
  WorkingDrumMapEntry();
  // Allocates _mapItem and copies dm to it.
  WorkingDrumMapEntry(const DrumMap& dm, fields_t fields);
  // Copy ctor.
  WorkingDrumMapEntry(const WorkingDrumMapEntry&);
  WorkingDrumMapEntry& operator=(const WorkingDrumMapEntry&);
};

typedef std::map < int /*index*/, WorkingDrumMapEntry, std::less<int> > WorkingDrumMapList_t;
typedef WorkingDrumMapList_t::iterator iWorkingDrumMapPatch_t;
typedef WorkingDrumMapList_t::const_iterator ciWorkingDrumMapPatch_t;
typedef std::pair<iWorkingDrumMapPatch_t, bool> WorkingDrumMapPatchInsertResult_t;
typedef std::pair<int, WorkingDrumMapEntry> WorkingDrumMapPatchInsertPair_t;
typedef std::pair<iWorkingDrumMapPatch_t, iWorkingDrumMapPatch_t> WorkingDrumMapPatchRangePair_t;

class WorkingDrumMapList : public WorkingDrumMapList_t {

  public:
    void add(int index,const WorkingDrumMapEntry& item);
    // Returns the requested fields that were NOT removed.
    int remove(int index, const WorkingDrumMapEntry& item);
    // Returns the requested fields that were NOT removed.
    int remove(int index, int fields);
    // If fillUnused is true it will fill any unused fields.
    void read(Xml&, bool fillUnused, int defaultIndex = -1);
    void write(int level, Xml&) const;
};


typedef std::map < int /*patch*/, WorkingDrumMapList, std::less<int> > WorkingDrumMapPatchList_t;
typedef WorkingDrumMapPatchList_t::iterator iWorkingDrumMapPatchList_t;
typedef WorkingDrumMapPatchList_t::const_iterator ciWorkingDrumMapPatchList_t;
typedef std::pair<iWorkingDrumMapPatchList_t, bool> WorkingDrumMapPatchListInsertResult_t;
typedef std::pair<int, WorkingDrumMapList> WorkingDrumMapPatchListInsertPair_t;

class WorkingDrumMapPatchList : public WorkingDrumMapPatchList_t {

  public:
    void add(const WorkingDrumMapPatchList& other);
    void add(int patch, const WorkingDrumMapList& list);
    void add(int patch, int index, const WorkingDrumMapEntry& item);
    void remove(int patch, int index, const WorkingDrumMapEntry& item, bool includeDefault);
    void remove(int patch, int index, int fields, bool includeDefault);
    void remove(int patch, bool includeDefault);
    WorkingDrumMapList* find(int patch, bool includeDefault);
    const WorkingDrumMapList* find(int patch, bool includeDefault) const;
    WorkingDrumMapEntry* find(int patch, int index, bool includeDefault);
    const WorkingDrumMapEntry* find(int patch, int index, bool includeDefault) const;

    // If fillUnused is true it will fill any unused items to ensure that all 128 items are filled.
    void read(Xml&, bool fillUnused);
    void write(int level, Xml&) const;
};


typedef std::map < std::string /*instrument name*/, WorkingDrumMapPatchList > WorkingDrumMapInstrumentList_t;
typedef WorkingDrumMapInstrumentList_t::iterator iWorkingDrumMapInstrumentList_t;
typedef WorkingDrumMapInstrumentList_t::const_iterator ciWorkingDrumMapInstrumentList_t;
typedef std::pair<iWorkingDrumMapInstrumentList_t, bool> WorkingDrumMapInstrumentListInsertResult_t;
typedef std::pair<std::string, WorkingDrumMapPatchList> WorkingDrumMapInstrumentListInsertPair_t;


class WorkingDrumMapInstrumentList : public WorkingDrumMapInstrumentList_t {
  public:
    void read(Xml&);
};

struct patch_drummap_mapping_t
{
  int _patch;
  DrumMap* drummap;
  int drum_in_map[128];

#ifdef _USE_INSTRUMENT_OVERRIDES_
  // A list of user-altered drum map items.
  WorkingDrumMapList _workingDrumMapList;
#endif

  // A NULL drummap is allowed, it means invalid (ie. invalid() returns true).
  patch_drummap_mapping_t(DrumMap* d, int patch = 0xffffff)
  {
    drummap = d;
    _patch = patch;
    update_drum_in_map();
  }

  patch_drummap_mapping_t(const patch_drummap_mapping_t& that);
  patch_drummap_mapping_t();
  ~patch_drummap_mapping_t();

  patch_drummap_mapping_t& operator=(const patch_drummap_mapping_t& that);

#ifdef _USE_INSTRUMENT_OVERRIDES_
  void addWorkingDrumMapEntry(int index,const WorkingDrumMapEntry& item);
  void removeWorkingDrumMapEntry(int index, const WorkingDrumMapEntry& item);
  void removeWorkingDrumMapEntry(int index, int fields);
#endif

  int map_drum_in(int enote) { return drum_in_map[enote]; }
  void update_drum_in_map();

  inline int hbank() const { return (_patch >> 16) & 0xff; }
  inline int lbank() const { return (_patch >> 8)  & 0xff; }
  inline int prog()  const { return  _patch & 0xff; }
  inline void setHBank(int v) { _patch = (_patch & 0x00ffff) | ((v & 0xff) << 16); }
  inline void setLBank(int v) { _patch = (_patch & 0xff00ff) | ((v & 0xff) << 8); }
  inline void setProg(int v)  { _patch = (_patch & 0xffff00) | (v & 0xff); }
  inline bool dontCare()        const { return hbankDontCare() && lbankDontCare() && programDontCare(); }
  inline bool hbankDontCare()   const { const int hb = hbank(); return hb < 0 || hb > 127; }
  inline bool lbankDontCare()   const { const int lb = lbank(); return lb < 0 || lb > 127; }
  inline bool programDontCare() const { const int pr = prog();  return pr < 0 || pr > 127; }

  bool isPatchInRange(int patch, bool includeDefault) const;

  bool isValid() const;

  QString to_string();
};

class patch_drummap_mapping_list_t : public std::list<patch_drummap_mapping_t>
{
  public:
    void add(const patch_drummap_mapping_list_t& other);
    void add(const patch_drummap_mapping_t& pdm);
    iterator find(int patch, bool includeDefault);
    const_iterator find(int patch, bool includeDefault) const;
    void read(Xml& xml);
    void write(int level, Xml&) const;
#ifdef _USE_INSTRUMENT_OVERRIDES_
    void writeDrummapOverrides(int level, Xml& xml) const;
#endif
};

typedef patch_drummap_mapping_list_t::iterator iPatchDrummapMapping_t;
typedef patch_drummap_mapping_list_t::const_iterator ciPatchDrummapMapping_t;


// NOTE: Channel == -1 (default) is legal.
typedef std::map < int /*channel*/, patch_drummap_mapping_list_t, std::less<int> > ChannelDrumMappingList_t;
typedef ChannelDrumMappingList_t::iterator iChannelDrumMappingList_t;
typedef ChannelDrumMappingList_t::const_iterator ciChannelDrumMappingList_t;
typedef std::pair<iChannelDrumMappingList_t, bool> ChannelDrumMappingListInsertResult_t;
typedef std::pair<int, patch_drummap_mapping_list_t> ChannelDrumMappingListInsertPair_t;

class ChannelDrumMappingList : public ChannelDrumMappingList_t {
  public:
    ChannelDrumMappingList();

    void add(const ChannelDrumMappingList& other);
    void add(int channel, const patch_drummap_mapping_list_t& list);
    //void add(int patch, int index, const WorkingDrumMapEntry& item);
    //void remove(int patch, int index, const WorkingDrumMapEntry& item, bool includeDefault);
    //void remove(int patch, int index, int fields, bool includeDefault);
    //void remove(int patch, bool includeDefault);
    patch_drummap_mapping_list_t* find(int channel, bool includeDefault);
    const patch_drummap_mapping_list_t* find(int channel, bool includeDefault) const;
    //WorkingDrumMapEntry* find(int patch, int index, bool includeDefault);
    //const WorkingDrumMapEntry* find(int patch, int index, bool includeDefault) const;

    // If fillUnused is true it will fill any unused items to ensure that all 128 items are filled.
    void read(Xml&);
    void write(int level, Xml&) const;
};



//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument {
   public:
     //              NoteOffAll  = Use all note offs.
     //              NoteOffNone = Do not use any note offs.
     // NoteOffConvertToZVNoteOn = Convert all note offs to zero-velocity note ons.
     enum NoteOffMode { NoteOffAll=0, NoteOffNone, NoteOffConvertToZVNoteOn };
      
   private:
      PatchGroupList pg;
      MidiControllerList* _controller;
      QList<SysEx*> _sysex;
      ChannelDrumMappingList _channelDrumMapping;
      bool _dirty;
      bool _waitForLSB; // Whether 14-bit controllers wait for LSB, or MSB and LSB are separate.
      NoteOffMode _noteOffMode;

      void init();

   protected:
      EventList* _midiInit;
      EventList* _midiReset;
      EventList* _midiState;
      // Set when loading midi state in SynthI::read, to indicate version 
      //  to SynthI::initInstance, which is called later. 
      int        _tmpMidiStateVersion; 
      char* _initScript;
      QString _name;
      QString _filePath;
      
      void writeDrummaps(int level, Xml& xml) const;
      void readDrummaps(Xml& xml);

   public:
      MidiInstrument();
      virtual ~MidiInstrument();
      MidiInstrument(const QString& txt);
      const QString& iname() const      { return _name; }
      void setIName(const QString& txt) { _name = txt; }
      MType midiType() const;
      virtual bool isSynti() const     { return false; }
      
      // Assign will 'delete' all existing patches and groups from the instrument.
      MidiInstrument& assign(const MidiInstrument&);
      QString filePath() const               { return _filePath;   }
      void setFilePath(const QString& s)     { _filePath = s;      }
      bool dirty() const                     { return _dirty;      }
      void setDirty(bool v)                  { _dirty = v;         }

      const QList<SysEx*>& sysex() const     { return _sysex; }
      void removeSysex(SysEx* sysex)         { _sysex.removeAll(sysex); }
      void addSysex(SysEx* sysex)            { _sysex.append(sysex); }


      QList<dumb_patchlist_entry_t> getPatches(int channel, bool drum);
      unsigned getNextPatch(int channel, unsigned patch, bool drum);
      unsigned getPrevPatch(int channel, unsigned patch, bool drum);
#ifdef _USE_INSTRUMENT_OVERRIDES_
      bool setWorkingDrumMapItem(int patch, int index, const WorkingDrumMapEntry&, bool isReset);
      // Returns OR'd WorkingDrumMapEntry::OverrideType flags indicating whether a map item's members,
      //  given by 'fields' (OR'd WorkingDrumMapEntry::Fields), are either the original or working map item.
      // Here in MidiInstrument the flags can be NoOverride and InstrumentOverride. See corresponding function
      //  in MidiTrack for additional TrackOverride flag use.
      int isWorkingMapItem(int patch, int index, int fields) const;
      void clearDrumMapOverrides();
#endif
      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType instrument overrides. Channel can be -1 meaning default.
      virtual void getMapItem(int channel, int patch, int index, DrumMap& dest_map,
                              int overrideType = WorkingDrumMapEntry::AllOverrides) const;

      EventList* midiInit() const            { return _midiInit; }
      EventList* midiReset() const           { return _midiReset; }
      EventList* midiState() const           { return _midiState; }
      const char* initScript() const         { return _initScript; }
      MidiControllerList* controller() const { return _controller; }
      bool waitForLSB() { return _waitForLSB; }
      void setWaitForLSB(bool v) { _waitForLSB = v; }
      
      // Virtual so that inheriters (synths etc) can return whatever they want.
      virtual NoteOffMode noteOffMode() const { return _noteOffMode; }
      // For non-synths, users can set this value.
      void setNoteOffMode(NoteOffMode mode) { _noteOffMode = mode; }

      void readMidiState(Xml& xml);
      virtual void reset(int); 
      virtual QString getPatchName(int channel, int prog, bool drum, bool includeDefault) const;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool);
      static void populateInstrPopup(MusEGui::PopupMenu*, int port, bool show_synths = false);  // Static
      void read(Xml&);
      void write(int level, Xml&);
#ifdef _USE_INSTRUMENT_OVERRIDES_
      void writeDrummapOverrides(int level, Xml&) const;
#endif

      PatchGroupList* groups()        { return &pg; }
      patch_drummap_mapping_list_t* get_patch_drummap_mapping(int channel, bool includeDefault);
      ChannelDrumMappingList* getChannelDrumMapping() { return &_channelDrumMapping; }
      };

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public std::list<MidiInstrument*> {

   public:
      MidiInstrumentList() {}
      iterator find(const MidiInstrument* instr);
#ifdef _USE_INSTRUMENT_OVERRIDES_
      void writeDrummapOverrides(int level, Xml&) const;
#endif
      };

typedef MidiInstrumentList::iterator iMidiInstrument;
typedef MidiInstrumentList::const_iterator ciMidiInstrument;

extern MidiInstrumentList midiInstruments;
extern MidiInstrument* genericMidiInstrument;
extern void initMidiInstruments();
extern MidiInstrument* registerMidiInstrument(const QString&);
extern void removeMidiInstrument(const QString& name);
extern void removeMidiInstrument(const MidiInstrument* instr);

} // namespace MusECore

#ifdef _USE_INSTRUMENT_OVERRIDES_
namespace MusEGlobal {
  extern MusECore::WorkingDrumMapInstrumentList workingDrumMapInstrumentList;
}
#endif

#endif

