//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.h,v 1.15 2006/01/14 17:08:48 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MINSTRUMENT_H__
#define __MINSTRUMENT_H__

#include "globaldefs.h"

class MidiOutPort;
class EventList;
class MidiControllerList;
class MidiController;
class MidiEvent;
class DrumMap;

//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch {
      signed char typ;                     // 1 - GM  2 - GS  4 - XG
      signed char hbank, lbank, prog;
      QString name;
      DrumMap* drumMap;
      void read(QDomNode, bool);

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
      void read(QDomNode);
      };

struct SysEx {
      QString name;
      QString comment;
      QString data;
      };

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument {
      std::vector<PatchGroup> pg;
      MidiControllerList* _controller;
      std::vector<SysEx> sysex;

      void init();

   protected:
      EventList* _midiInit;
      EventList* _midiReset;
      EventList* _midiState;
      char* _initScript;
      QString _name;

   public:
      MidiInstrument();
      virtual ~MidiInstrument();
      MidiInstrument(const QString& txt);
      const QString& iname() const           { return _name; }
      void setIName(const QString& txt)      { _name = txt; }

      EventList* midiInit() const            { return _midiInit; }
      EventList* midiReset() const           { return _midiReset; }
      EventList* midiState() const           { return _midiState; }
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
      std::vector<SysEx>* sysexList()     { return &sysex; }
      MidiController* midiController(int num) const;
      void reset(MidiOutPort*);

      std::vector<PatchGroup>* groups() { return &pg; }
      };

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public std::vector<MidiInstrument*> {

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

