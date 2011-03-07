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

#ifndef __MINSTRUMENT_H__
#define __MINSTRUMENT_H__

#include "globaldefs.h"

namespace AL {
      class Xml;
      };
using AL::Xml;

class MidiOutPort;
class EventList;
class MidiControllerList;
class MidiController;
class MidiEvent;
class DrumMap;
class MidiInstrument;

//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch {
      signed char typ;                     // 1 - GM  2 - GS  4 - XG
      signed char hbank, lbank, prog;
      QString name;
      int categorie;
      DrumMap* drumMap;

      void read(QDomNode, bool, MidiInstrument*);
      void write(Xml& xml);

      Patch();
      ~Patch();
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
      void read(QDomNode, MidiInstrument*);
      };

struct SysEx {
      QString name;
      QString comment;
      int dataLen;
      unsigned char* data;
      };

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument {
      std::vector<PatchGroup> pg;
      MidiControllerList* _controller;
      QList<SysEx*> _sysex;
      bool _dirty;
      bool _readonly;

      void init();

   protected:
      QList<QString> _categories;
      EventList* _midiInit;
      EventList* _midiReset;
      EventList* _midiState;
      char* _initScript;
      QString _name;
      QString _filePath;

   public:
      MidiInstrument();
      virtual ~MidiInstrument();
      MidiInstrument(const QString& txt);
      const QString& iname() const           { return _name;       }
      void setIName(const QString& txt)      { _name = txt;        }
      QString filePath() const               { return _filePath;   }
      void setFilePath(const QString& s)     { _filePath = s;      }
      bool dirty() const                     { return _dirty;      }
      void setDirty(bool v)                  { _dirty = v;         }
      bool readonly() const                  { return _readonly;   }
      void setReadonly(bool v)               { _readonly = v;      }

      EventList* midiInit() const            { return _midiInit;   }
      EventList* midiReset() const           { return _midiReset;  }
      EventList* midiState() const           { return _midiState;  }
      const char* initScript() const         { return _initScript; }
      MidiControllerList* controller() const { return _controller; }

      void readMidiState(QDomNode);
      virtual bool guiVisible() const        { return false; }
      virtual void showGui(bool)             {}
      virtual bool hasGui() const            { return false; }
      virtual void writeToGui(const MidiEvent&) {}

      virtual QString getPatchName(int,int);
      virtual DrumMap* getDrumMap(int);

      virtual void populatePatchPopup(QMenu*, int);
      void read(QDomNode);
      void write(Xml& xml);
      const QList<SysEx*>& sysex() const    { return _sysex; }
      void removeSysex(SysEx* sysex)        { _sysex.removeAll(sysex); }
      void addSysex(SysEx* sysex)           { _sysex.append(sysex); }

      MidiController* midiController(int num) const;

      std::vector<PatchGroup>* groups()        { return &pg; }
      const QList<QString>& categories() const { return _categories; }
      void addCategory(const QString& s)       { _categories.append(s); }
      };

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public QList<MidiInstrument*> {
      };

typedef MidiInstrumentList::iterator iMidiInstrument;

extern MidiInstrumentList midiInstruments;

extern MidiInstrument* genericMidiInstrument;
extern void initMidiInstruments();
extern MidiInstrument* registerMidiInstrument(const QString&);
extern void removeMidiInstrument(const QString& name);
extern void removeMidiInstrument(const MidiInstrument* instr);

#endif

