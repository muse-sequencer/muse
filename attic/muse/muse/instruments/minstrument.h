//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.h,v 1.3.2.3 2009/03/09 02:05:18 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MINSTRUMENT_H__
#define __MINSTRUMENT_H__

#include "globaldefs.h"
#include <list>
#include <vector>

class MidiPort;
class QPopupMenu;
class MidiPlayEvent;
class Xml;
class EventList;
class MidiControllerList;
class QString;

//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch {
      signed char typ;                     // 1 - GM  2 - GS  4 - XG
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

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument {
      PatchGroupList pg;
      MidiControllerList* _controller;
      bool _dirty;
      int _nullvalue;

      void init();

   protected:
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
      const QString& iname() const      { return _name; }
      void setIName(const QString& txt) { _name = txt; }
      
      //MidiInstrument& uniqueCopy(const MidiInstrument&);
      // Assign will 'delete' all existing patches and groups from the instrument.
      MidiInstrument& assign(const MidiInstrument&);
      QString filePath() const               { return _filePath;   }
      void setFilePath(const QString& s)     { _filePath = s;      }
      bool dirty() const                     { return _dirty;      }
      void setDirty(bool v)                  { _dirty = v;         }


      EventList* midiInit() const            { return _midiInit; }
      EventList* midiReset() const           { return _midiReset; }
      EventList* midiState() const           { return _midiState; }
      const char* initScript() const         { return _initScript; }
      MidiControllerList* controller() const { return _controller; }
      int nullSendValue() { return _nullvalue; }
      void setNullSendValue(int v) { _nullvalue = v; }

      void readMidiState(Xml& xml);
      virtual bool guiVisible() const   { return false; }
      virtual void showGui(bool)        {}
      virtual bool hasGui() const       { return false; }
      virtual void writeToGui(const MidiPlayEvent&) {}

      virtual void reset(int, MType);
      virtual const char* getPatchName(int,int,MType,bool);
      virtual void populatePatchPopup(QPopupMenu*, int, MType, bool);
      void read(Xml&);
      void write(int level, Xml&);
      PatchGroupList* groups()        { return &pg; }
      };

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public std::list<MidiInstrument*> {

   public:
      MidiInstrumentList() {}
      };

typedef MidiInstrumentList::iterator iMidiInstrument;

extern MidiInstrumentList midiInstruments;
extern MidiInstrument* genericMidiInstrument;
extern void initMidiInstruments();
extern MidiInstrument* registerMidiInstrument(const QString&);
extern void removeMidiInstrument(const QString& name);
extern void removeMidiInstrument(const MidiInstrument* instr);

#endif

