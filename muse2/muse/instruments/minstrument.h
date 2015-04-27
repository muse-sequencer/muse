//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.h,v 1.3.2.3 2009/03/09 02:05:18 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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
#include <list>
#include <vector>
#include <QString>

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {
class EventList;
class MidiControllerList;
class MidiPort;
class MidiPlayEvent;
class Xml;
struct DrumMap;


//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch {
      // REMOVE Tim. OBSOLETE. 
      //signed char typ;                     // 1 - GM  2 - GS  4 - XG
      signed char hbank, lbank, prog;
      QString name;
      bool drum;
      void read(Xml&);
      void write(int level, Xml&);
      };

typedef std::list<Patch*> PatchList;
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

typedef std::vector<PatchGroup*> PatchGroupList;
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

struct patch_collection_t
{
  int first_program;
  int last_program;
  int first_hbank;
  int last_hbank;
  int first_lbank;
  int last_lbank;
  
  patch_collection_t(int p1=0, int p2=127, int l1=0, int l2=127, int h1=0, int h2=127)
  {
    first_program=p1;
    last_program=p2;
    first_lbank=l1;
    last_lbank=l2;
    first_hbank=h1;
    last_hbank=h2;
  }
  
  QString to_string();
};

struct patch_drummap_mapping_t
{
  patch_collection_t affected_patches;
  DrumMap* drummap;
  
  patch_drummap_mapping_t(const patch_collection_t& a, DrumMap* d)
  {
    affected_patches=a;
    drummap=d;
  }
  
  patch_drummap_mapping_t(const patch_drummap_mapping_t& that);
  patch_drummap_mapping_t();
  ~patch_drummap_mapping_t();
  
  patch_drummap_mapping_t& operator=(const patch_drummap_mapping_t& that);
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

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument {
      PatchGroupList pg;
      MidiControllerList* _controller;
      QList<SysEx*> _sysex;
      std::list<patch_drummap_mapping_t> patch_drummap_mapping;
      bool _dirty;
      int _nullvalue;

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
      patch_drummap_mapping_t readDrummapsEntry(Xml& xml);
      patch_collection_t readDrummapsEntryPatchCollection(Xml& xml);

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
      
      const DrumMap* drummap_for_patch(int patch) const;
      QList<dumb_patchlist_entry_t> getPatches(int channel, bool drum);
      unsigned getNextPatch(int channel, unsigned patch, bool drum);
      unsigned getPrevPatch(int channel, unsigned patch, bool drum);
      
      EventList* midiInit() const            { return _midiInit; }
      EventList* midiReset() const           { return _midiReset; }
      EventList* midiState() const           { return _midiState; }
      const char* initScript() const         { return _initScript; }
      MidiControllerList* controller() const { return _controller; }
      int nullSendValue() { return _nullvalue; }
      void setNullSendValue(int v) { _nullvalue = v; }

      void readMidiState(Xml& xml);
      // REMOVE Tim. Persistent routes. Removed.
//       virtual bool guiVisible() const   { return false; }
//       virtual void showGui(bool)        {}
//       virtual bool hasGui() const       { return false; }
//       virtual bool nativeGuiVisible() const   { return false; }
//       virtual void showNativeGui(bool)        {}
//       virtual bool hasNativeGui() const       { return false; }
//       virtual void writeToGui(const MidiPlayEvent&) {}

      virtual void reset(int); 
      virtual QString getPatchName(int,int,bool) const;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool);
      static void populateInstrPopup(MusEGui::PopupMenu*, MidiInstrument* current = 0, bool show_synths = false);  // Static
      void read(Xml&);
      void write(int level, Xml&);
      
      PatchGroupList* groups()        { return &pg; }
      std::list<patch_drummap_mapping_t>* get_patch_drummap_mapping() { return &patch_drummap_mapping; }
      };

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public std::list<MidiInstrument*> {

   public:
      MidiInstrumentList() {}
      iterator find(const MidiInstrument* instr);
      };

typedef MidiInstrumentList::iterator iMidiInstrument;

extern MidiInstrumentList midiInstruments;
extern MidiInstrument* genericMidiInstrument;
extern void initMidiInstruments();
extern MidiInstrument* registerMidiInstrument(const QString&);
extern void removeMidiInstrument(const QString& name);
extern void removeMidiInstrument(const MidiInstrument* instr);

} // namespace MusECore

#endif

