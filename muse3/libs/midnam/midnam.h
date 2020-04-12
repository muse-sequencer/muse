//=========================================================
//  MusE
//  Linux Music Editor
//
//  midnam.h
//  (C) Copyright 2019, 2020 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MIDNAM_H__
#define __MIDNAM_H__

#include <map>
#include <set>
#include <list>
#include <QString>

#include "xml.h"
#include "mpevent.h"
#include "midi_controller.h"

namespace MusECore {

struct MidNamReferencesList;

class MidiNamMIDICommands : public MPEventList
{
  private:
    bool _isPatchMIDICommands;
    // If bank high or low control commands are detected,
    //  these values are set to them. Otherwise they are
    //  set at 'unknown' (default) 0xff.
    int _bankH;
    int _bankL;
    bool _hasBankH;
    bool _hasBankL;

  public:
    MidiNamMIDICommands() :
      _isPatchMIDICommands(false), _bankH(0xff), _bankL(0xff), _hasBankH(false), _hasBankL(false)  { }
    bool isPatchMIDICommands() const { return _isPatchMIDICommands; }
    void setIsPatchMIDICommands(bool v) { _isPatchMIDICommands = v; }
    int bankH() const { return _bankH; }
    int bankL() const { return _bankL; }
    bool hasBankH() const { return _hasBankH; }
    bool hasBankL() const { return _hasBankL; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(
      MusECore::Xml& xml,
      bool includeSysEx = true,
      int defaultPort = 0,
      bool channelRequired = false,
      int defaultChannel = 0);
};


//-------------------------------------------------------------------


class MidiNamAvailableChannel
{
  private:
    int _channel;
    bool _available;

  public:
    MidiNamAvailableChannel() : _channel(0), _available(false) { }
    MidiNamAvailableChannel(int channel, bool available) : _channel(channel), _available(available) { }
    int channel() const { return _channel; }
    bool available() const { return _available; }
    void setAvailable(bool available) { 
        _available = available; }
    bool operator<(const MidiNamAvailableChannel& n) const { return _channel < n._channel; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-------------------------------------------------------------------


class MidiNamAvailableForChannels : public std::map<int /* channel */, MidiNamAvailableChannel*, std::less<int>>
{
  public:
    MidiNamAvailableForChannels() { }
    MidiNamAvailableForChannels(const MidiNamAvailableForChannels& m);
    ~MidiNamAvailableForChannels();
    bool add(MidiNamAvailableChannel* a);
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamAvailableForChannels::iterator iMidiNamAvailableForChannels;
typedef MidiNamAvailableForChannels::const_iterator ciMidiNamAvailableForChannels;
typedef std::pair<int /* channel */, MidiNamAvailableChannel*> MidiNamAvailableForChannelsPair;


//-------------------------------------------------------------------


class MidNamChannelNameSet;
class MidiNamPatch;
class MidiNamPatchBankList;
class MidiNamChannelNameSetAssign
{
  private:
    int _channel;
    QString _name;

    // Points to a reference.
    MidNamChannelNameSet* _p_ref;

  public:
    MidiNamChannelNameSetAssign() : _channel(0), _p_ref(nullptr) { }
    MidiNamChannelNameSetAssign(int channel, const QString& nameSet) :
      _channel(channel), _name(nameSet), _p_ref(nullptr) { }
    int channel() const { return _channel; }
    const QString& name() const { return _name; }
    void setName(const QString& nameSet) { 
        _name = nameSet; }
    // NOTE: Unlike the other referencing classes, this always returns the reference object and can be NULL.
    MidNamChannelNameSet* objectOrRef() { return  _p_ref; }
    void setObjectOrRef(MidNamChannelNameSet* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool operator<(const MidiNamChannelNameSetAssign& n) const { return _channel < n._channel; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidiNamChannelNameSetAssignments : public std::map<int /* channel */, MidiNamChannelNameSetAssign*, std::less<int>>
{
  private:
    bool _hasChannelNameSetAssignments;

  public:
    MidiNamChannelNameSetAssignments() : _hasChannelNameSetAssignments(false) { }
    MidiNamChannelNameSetAssignments(const MidiNamChannelNameSetAssignments& m);
    ~MidiNamChannelNameSetAssignments();
    bool hasChannelNameSetAssignments() const { return _hasChannelNameSetAssignments; }
    bool add(MidiNamChannelNameSetAssign* a);
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};
typedef MidiNamChannelNameSetAssignments::iterator iMidiNamChannelNameSetAssignments;
typedef MidiNamChannelNameSetAssignments::const_iterator ciMidiNamChannelNameSetAssignments;
typedef std::pair<int /* channel */, MidiNamChannelNameSetAssign*> MidiNamChannelNameSetAssignmentsPair;


//-------------------------------------------------------------------


class MidiNamNotes;

class MidiNamNoteGroup : public std::set<int /* note number */>
{
  private:
    // Optional.
    QString _name;

  public:
    MidiNamNoteGroup() {}
    MidiNamNoteGroup(const QString& name) : _name(name) { }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamNoteGroup& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml, const MidiNamNotes* notes) const;
    void read(MusECore::Xml& xml, MidiNamNotes* notes);
};
typedef MidiNamNoteGroup::iterator iMidiNamNoteGroup;
typedef MidiNamNoteGroup::const_iterator ciMidiNamNoteGroup;
typedef std::pair<iMidiNamNoteGroup, bool> MidiNamNoteGroupPair;

// The note group name is optional, may be blank.
class MidiNamNoteGroups : public std::multimap<QString /* name */, MidiNamNoteGroup*, std::less<QString>>
{
  public:
    MidiNamNoteGroups() { }
    MidiNamNoteGroups(const MidiNamNoteGroups& m);
    ~MidiNamNoteGroups();
    MidiNamNoteGroups& operator=(const MidiNamNoteGroups& m);
    bool add(MidiNamNoteGroup* a);
    void write(int level, MusECore::Xml& xml, const MidiNamNotes* notes) const;
};
typedef MidiNamNoteGroups::iterator iMidiNamNoteGroups;
typedef MidiNamNoteGroups::const_iterator ciMidiNamNoteGroups;
typedef std::pair<QString /* name */, MidiNamNoteGroup*> MidiNamNoteGroupsPair;
typedef std::pair<iMidiNamNoteGroups, iMidiNamNoteGroups> MidiNamNoteGroupsRange;


//-------------------------------------------------------------------


class MidiNamNote
{
  private:
    int _number;
    QString _name;

  public:
    MidiNamNote() : _number(0) { }
    MidiNamNote(int number, const QString& name) : _number(number), _name(name) { }
    int number() const { return _number; }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamNote& n) const { return _number < n._number; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamNotes : public std::map<int /* number */, MidiNamNote*, std::less<int>>
{
  private:
    MidiNamNoteGroups _noteGroups;

  public:
    MidiNamNotes() { }
    MidiNamNotes(const MidiNamNotes& m);
    ~MidiNamNotes();
    MidiNamNoteGroups& noteGroups() { return _noteGroups; }
    const MidiNamNoteGroups& noteGroups() const { return _noteGroups; }
    bool isEmpty() const { return _noteGroups.empty() && empty(); }
    bool add(MidiNamNote* a);
    bool addNoteGroup(MidiNamNoteGroup* a);
    void write(int level, MusECore::Xml& xml) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};
typedef MidiNamNotes::iterator iMidiNamNotes;
typedef MidiNamNotes::const_iterator ciMidiNamNotes;
typedef std::pair<int /* number */, MidiNamNote*> MidiNamNotesPair;
typedef std::pair<iMidiNamNotes, bool> MidiNamNotesInsPair;


//-------------------------------------------------------------------


class MidNamNoteNameList
{
  private:
    QString _name;
    MidiNamNotes _noteList;

    // Points to a reference.
    MidNamNoteNameList* _p_ref;
    bool _isReference;

    bool _hasNoteNameList;

  public:
    MidNamNoteNameList() : _p_ref(nullptr), _isReference(false), _hasNoteNameList(false) { }
    MidNamNoteNameList(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false), _hasNoteNameList(false) { }
    bool hasNoteNameList() const { return _hasNoteNameList; }
    // Outside of these classes, always use this method to get the real list.
    const MidNamNoteNameList* objectOrRef() const { return (_isReference && _p_ref) ? _p_ref : this; }
    MidNamNoteNameList* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidNamNoteNameList* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool empty() const { return _noteList.isEmpty(); }
    const MidiNamNotes& noteList() const { return _noteList; }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    bool operator<(const MidNamNoteNameList& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};


//-------------------------------------------------------


class MidiNamVal
{
  private:
    int _number;
    QString _name;

  public:
    MidiNamVal() : _number(0) { }
    MidiNamVal(int number, const QString& name) : _number(number), _name(name) { }
    int number() const { return _number; }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamVal& n) const { return _number < n._number; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamValNames : public std::map<int /* number */, MidiNamVal*, std::less<int>>
{
  private:
    QString _name;
    // Points to a reference.
    MidiNamValNames* _p_ref;
    bool _isReference;

  public:
    MidiNamValNames() : _p_ref(nullptr), _isReference(false) {}
    MidiNamValNames(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false) { }
    MidiNamValNames(const MidiNamValNames& m);
    MidiNamValNames& operator=(const MidiNamValNames& m);
    ~MidiNamValNames();
    MidiNamValNames* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidiNamValNames* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool add(MidiNamVal* a);
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamValNames::iterator iMidiNamValNames;
typedef MidiNamValNames::const_iterator ciMidiNamValNames;
typedef std::pair<int /* number */, MidiNamVal*> MidiNamValNamesPair;
typedef std::pair<iMidiNamValNames, bool> MidiNamValNamesInsPair;


class MidiNamValues
{
  private:
    int _min;
    int _max;
    int _default;
    int _units;
    int _mapping;
    MidiNamValNames _valueNames;

  public:
    MidiNamValues() : _min(0), _max(127), _default(0), _units(0), _mapping(0) {}
    MidiNamValues(int min, int max, int def, int units, int mapping) :
                _min(min), _max(max), _default(def), _units(units),
                _mapping(mapping) {}
    int minVal() const { return _min; }
    void setMinVal(int v) { _min = v; }
    int maxVal() const { return _max; }
    void setMaxVal(int v) { _max = v; }
    int defaultVal() const { return _default; }
    void setDefaultVal(int v) { _default = v; }
    int units() const { return _units; }
    void setUnits(int v) { _units = v; }
    int mapping() const { return _mapping; }
    void setMapping(int v) { _mapping = v; }
    MidiNamValNames& valueNames() { return _valueNames; }
    bool empty() const { return _valueNames.empty(); }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-------------------------------------------------------


class MidiNamCtrl : public MidiController
{
  private:
    MidiNamValues _values;

  public:
    MidiNamCtrl() : MidiController() {}
    MidiNamCtrl(int number, const QString& name) :
                MidiController(name, number, 0, 127, 0, -1) {}
    bool operator<(const MidiNamCtrl& n) const { return _num < n._num; }
    void writeMidnam(int level, MusECore::Xml& xml) const;
    bool readMidnam(MusECore::Xml& xml);
};

// REMOVE Tim. midnam. Changed.
class MidiNamCtrls : public MidiControllerList
//class MidiNamCtrls : public CompoundMidiControllerList_t
{
  private:
    QString _name;

    // Points to a reference.
    MidiNamCtrls* _p_ref;
    bool _isReference;

  public:
    MidiNamCtrls() : _p_ref(nullptr), _isReference(false) { }
    MidiNamCtrls(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false) { }
    MidiNamCtrls(const MidiNamCtrls& mcl);
    // We require a destructor here because MidiControllerList
    //  does not delete its contents.
    ~MidiNamCtrls();

    const MidiNamCtrls* objectOrRef() const { return (_isReference && _p_ref) ? _p_ref : this; }
    MidiNamCtrls* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidiNamCtrls* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    void writeMidnam(int level, MusECore::Xml& xml) const;
    void readMidnam(MusECore::Xml& xml);

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;
};
// typedef MidiNamCtrls::iterator iMidiNamCtrl;
// typedef MidiNamCtrls::const_iterator ciMidiNamCtrl;
// typedef std::pair<iMidiNamCtrl, bool> MidiNamCtrlPair;


//-----------------------------------------------------------------


class MidiNamPatch
{
  private:
    QString _number;
    QString _name;
    int _patchNumber;

    MidiNamMIDICommands _patchMIDICommands;

    MidiNamChannelNameSetAssignments _channelNameSetAssignments;

    MidNamNoteNameList _noteNameList;
    MidiNamCtrls _controlNameList;
    
  public:
    MidiNamPatch(int patchNumber = CTRL_PROGRAM_VAL_DONT_CARE) : _patchNumber(patchNumber) {}
    MidiNamPatch(const QString& number, const QString& name, int patchNumber = CTRL_PROGRAM_VAL_DONT_CARE) :
                _number(number), _name(name), _patchNumber(patchNumber) {}
    int patchNumber() const { return _patchNumber; }
    MidiNamMIDICommands& patchMIDICommands() { return _patchMIDICommands; }
    const MidiNamMIDICommands& patchMIDICommands() const { return _patchMIDICommands; }
    MidiNamChannelNameSetAssignments& channelNameSetAssignments() { return _channelNameSetAssignments; }
    const MidiNamChannelNameSetAssignments& channelNameSetAssignments() const { return _channelNameSetAssignments; }
    MidNamNoteNameList& noteNameList() { return _noteNameList; }
    const MidNamNoteNameList& noteNameList() const { return _noteNameList; }
    MidiNamCtrls& controlNameList() { return _controlNameList; }
    const MidiNamCtrls& controlNameList() const { return _controlNameList; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamPatch& n) const { return _patchNumber < n._patchNumber; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidiNamPatchNameList : public std::map<int /* patchNumber */, MidiNamPatch*, std::less<int>>
{
  private:
    // Optional.
    QString _name;

    // Points to a reference.
    MidiNamPatchNameList* _p_ref;
    bool _isReference;

  public:
    MidiNamPatchNameList() : _p_ref(nullptr), _isReference(false) {}
    MidiNamPatchNameList(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false) { }
    MidiNamPatchNameList(const MidiNamPatchNameList& m);
    ~MidiNamPatchNameList();
    bool add(MidiNamPatch* a);
    const MidiNamPatchNameList* objectOrRef() const { return (_isReference && _p_ref) ? _p_ref : this; }
    MidiNamPatchNameList* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidiNamPatchNameList* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    bool operator<(const MidiNamPatchNameList& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int patch, int bankHL = 0xffff) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name, int bankHL = 0xffff) const;
};
typedef MidiNamPatchNameList::iterator iMidiNamPatchNameList;
typedef MidiNamPatchNameList::const_iterator ciMidiNamPatchNameList;
typedef std::pair<int /* patchNumber */, MidiNamPatch*> MidiNamPatchNameListPair;


//-----------------------------------------------------------------


class MidiNamPatchBank
{
  private:
    QString _name;
    bool _ROM;

    MidiNamMIDICommands _MIDICommands;
    // If bank high or low control commands are detected,
    //  this value is set to them. Otherwise it is
    //  set at 'unknown' (default) bank high and low 0xffff.
    int _bankHL;

    MidiNamPatchNameList _patchNameList;

  public:
    MidiNamPatchBank() : _ROM(false), _bankHL(0xffff) {}
    MidiNamPatchBank(const QString& name, bool ROM) :
                _name(name), _ROM(ROM), _bankHL(0xffff) {}
    MidiNamPatchNameList& patchNameList() { return _patchNameList; }
    const MidiNamPatchNameList& patchNameList() const { return _patchNameList; }
    MidiNamMIDICommands& MIDICommands() { return _MIDICommands; }
    const MidiNamMIDICommands& MIDICommands() const { return _MIDICommands; }
    int bankHL() const { return _bankHL; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamPatchBank& n) const { return _bankHL < n._bankHL; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int patch) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidiNamPatchBankList : public std::map<int /* bankHL */, MidiNamPatchBank*, std::less<int>>
{
  public:
    MidiNamPatchBankList() { }
    MidiNamPatchBankList(const MidiNamPatchBankList& m);
    ~MidiNamPatchBankList();
    bool add(MidiNamPatchBank* a);
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int patch) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};
typedef MidiNamPatchBankList::iterator iMidiNamPatchBankList;
typedef MidiNamPatchBankList::const_iterator ciMidiNamPatchBankList;
typedef std::pair<int /* bankHL */, MidiNamPatchBank*> MidiNamPatchBankListPair;


//-----------------------------------------------------------------

class MidNamChannelNameSet
{
  private:
    QString _name;
    
    MidiNamAvailableForChannels _availableForChannels;

    MidNamNoteNameList _noteNameList;
    MidiNamCtrls _controlNameList;
    MidiNamPatchBankList _patchBankList;

  public:
    MidNamChannelNameSet() {}
    MidNamChannelNameSet(
      const QString& name) :
      _name(name) {}
    const QString& name() const { return _name; }
    void setName(const QString& nameSet) { 
        _name = nameSet; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool operator<(const MidNamChannelNameSet& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidiNamChannelNameSetList : public std::map<QString /* name */, MidNamChannelNameSet*, std::less<QString>>
{
  public:
    MidiNamChannelNameSetList() { }
    MidiNamChannelNameSetList(const MidiNamChannelNameSetList& m);
    ~MidiNamChannelNameSetList();
    bool add(MidNamChannelNameSet* a);
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};
typedef MidiNamChannelNameSetList::iterator iMidiNamChannelNameSetList;
typedef MidiNamChannelNameSetList::const_iterator ciMidiNamChannelNameSetList;
typedef std::pair<QString /* name */, MidNamChannelNameSet*> MidiNamChannelNameSetListPair;


//-----------------------------------------------------------------


class MidNamDeviceModeEnable
{
  private:
    MidiNamMIDICommands _MIDICommands;

  public:
    MidNamDeviceModeEnable() {}
    const MidiNamMIDICommands& MIDICommands() const { return _MIDICommands; }
    MidiNamMIDICommands& MIDICommands() { return _MIDICommands; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidNamDeviceModeDisable
{
  private:
    MidiNamMIDICommands _MIDICommands;

  public:
    MidNamDeviceModeDisable() {}
    const MidiNamMIDICommands& MIDICommands() const { return _MIDICommands; }
    MidiNamMIDICommands& MIDICommands() { return _MIDICommands; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-----------------------------------------------------------------


class MidNamNameList
{
  private:
    MidiNamPatchNameList _patchNameList;
    MidNamNoteNameList _noteNameList;
    MidiNamCtrls _controlNameList;
    MidiNamValNames _valueNameList;
    
  public:
    MidNamNameList() {}
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool empty() const {
      return _patchNameList.empty() &&
      _noteNameList.empty() &&
      _controlNameList.empty() &&
      _valueNameList.empty();
    }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-----------------------------------------------------------------


class MidNamDeviceMode
{
  private:
    QString _name;
    // Whether this is a StandardDeviceMode or CustomDeviceMode.
    bool _isCustomDeviceMode;

    MidNamDeviceModeEnable _deviceModeEnable;
    MidNamDeviceModeDisable _deviceModeDisable;

    MidiNamChannelNameSetAssignments _channelNameSetAssignments;
    MidNamNameList _nameList;
    // Used only with StandardDeviceMode.
    MidiNamChannelNameSetList _channelNameSetList;

    // Points to a reference (a SupportsStandardDeviceMode reference to a StandardDeviceMode).
    MidNamDeviceMode* _p_ref;
    bool _isReference;

  public:
    MidNamDeviceMode() : _isCustomDeviceMode(false), _p_ref(nullptr), _isReference(false) {}
    MidNamDeviceMode(
      const QString& name) :
      _name(name), _isCustomDeviceMode(false), _p_ref(nullptr), _isReference(false) {}
    const MidNamDeviceMode* objectOrRef() const { return (_isReference && _p_ref) ? _p_ref : this; }
    MidNamDeviceMode* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidNamDeviceMode* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool isCustomDeviceMode() const { return _isCustomDeviceMode; }
    void setCustomDeviceMode(bool v) { _isCustomDeviceMode = v; }
    const MidNamDeviceModeEnable& deviceModeEnable() const { return _deviceModeEnable; }
    MidNamDeviceModeEnable& deviceModeEnable() { return _deviceModeEnable; }
    const MidNamDeviceModeDisable& deviceModeDisable() const { return _deviceModeDisable; }
    MidNamDeviceModeDisable& deviceModeDisable() { return _deviceModeDisable; }
    const MidiNamChannelNameSetAssignments& channelNameSetAssignments() const { return _channelNameSetAssignments; }
    MidiNamChannelNameSetAssignments& channelNameSetAssignments() { return _channelNameSetAssignments; }
    const MidNamNameList& nameList() const { return _nameList; }
    MidNamNameList& nameList() { return _nameList; }
    const MidiNamChannelNameSetList& channelNameSetList() const { return _channelNameSetList; }
    MidiNamChannelNameSetList& channelNameSetList() { return _channelNameSetList; }
    const QString& name() const { return _name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    bool operator<(const MidNamDeviceMode& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamDeviceModeList : public std::map<QString /* name */, MidNamDeviceMode*, std::less<QString>>
{
  public:
    MidNamDeviceModeList() { }
    MidNamDeviceModeList(const MidNamDeviceModeList& m);
    ~MidNamDeviceModeList();
    bool add(MidNamDeviceMode* a);
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidNamDeviceModeList::iterator iMidNamDeviceModeList;
typedef MidNamDeviceModeList::const_iterator ciMidNamDeviceModeList;
typedef std::pair<QString /* name */, MidNamDeviceMode*> MidNamDeviceModeListPair;


//-----------------------------------------------------------------


class MidNamDevice
{
  private:
    QString _name;
    int _uniqueID;

  public:
    MidNamDevice() : _uniqueID(0) {}
    MidNamDevice(const QString& name, int uniqueID) :
                _name(name), _uniqueID(uniqueID) {}
    const QString& name() const { return _name; }
    int uniqueID() const { return _uniqueID; }
    bool operator<(const MidNamDevice& n) const { return _uniqueID < n._uniqueID; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-----------------------------------------------------------------


class MidNamModel
{
  private:
    QString _model;

  public:
    MidNamModel() {}
    MidNamModel(const QString& model) :
                _model(model) {}
    const QString& model() const { return _model; }
    bool operator<(const MidNamModel& n) const { return _model < n._model; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamModelList : public std::map<QString /* model */, MidNamModel*, std::less<QString>>
{
  public:
    MidiNamModelList() { }
    MidiNamModelList(const MidiNamModelList& m);
    ~MidiNamModelList();
    bool add(MidNamModel* a);
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamModelList::iterator iMidiNamModelList;
typedef MidiNamModelList::const_iterator ciMidiNamModelList;
typedef std::pair<QString /* model */, MidNamModel*> MidiNamModelListPair;


//-----------------------------------------------------------------


class MidNamManufacturer
{
  private:
    QString _manufacturer;

  public:
    MidNamManufacturer() {}
    MidNamManufacturer(const QString& manufacturer) :
                _manufacturer(manufacturer) {}
    const QString& manufacturer() const { return _manufacturer; }
    bool operator<(const MidNamManufacturer& n) const { return _manufacturer < n._manufacturer; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-----------------------------------------------------------------


class MidNamAuthor
{
  private:
    QString _author;

  public:
    MidNamAuthor() {}
    MidNamAuthor(const QString& author) :
                _author(author) {}
    const QString& author() const { return _author; }
    void clear() { _author.clear(); }
    bool operator<(const MidNamAuthor& n) const { return _author < n._author; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-----------------------------------------------------------------


class MidNamExtendingDeviceNames
{
  private:
    MidNamManufacturer _manufacturer;
    MidiNamModelList _modelList;
    MidNamDevice _device;
    MidNamNameList _nameList;

  public:
    MidNamExtendingDeviceNames() {}
    bool gatherReferences(MidNamReferencesList* refs) const;
    MidNamManufacturer& manufacturer() { return _manufacturer; }
    MidiNamModelList& modelList() { return _modelList; }
    MidNamDevice& device() { return _device; }
    MidNamNameList& nameList() { return _nameList; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidNamExtendingDeviceNamesList : public std::list<MidNamExtendingDeviceNames*>
{
  public:
    MidNamExtendingDeviceNamesList() { }
    MidNamExtendingDeviceNamesList(const MidNamExtendingDeviceNamesList& m);
    ~MidNamExtendingDeviceNamesList();
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidNamExtendingDeviceNamesList::iterator iMidNamExtendingDeviceNamesList;
typedef MidNamExtendingDeviceNamesList::const_iterator ciMidNamExtendingDeviceNamesList;


//-----------------------------------------------------------------


class MidNamMasterDeviceNames
{
  private:
    MidNamManufacturer _manufacturer;
    MidiNamModelList _modelList;
    MidNamDevice _device;
    MidNamDeviceModeList _deviceModeList;
    MidiNamChannelNameSetList _channelNameSetList;
    MidNamNameList _nameList;

  public:
    MidNamMasterDeviceNames() {}
    bool gatherReferences(MidNamReferencesList* refs) const;
    MidNamManufacturer& manufacturer() { return _manufacturer; }
    MidiNamModelList& modelList() { return _modelList; }
    MidNamDevice& device() { return _device; }
    const MidNamDeviceModeList* deviceModeList() const { return &_deviceModeList; }
    MidiNamChannelNameSetList& channelNameSetList() { return _channelNameSetList; }
    MidNamNameList& nameList() { return _nameList; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamMasterDeviceNamesList : public std::list<MidNamMasterDeviceNames*>
{
  public:
    MidNamMasterDeviceNamesList() { }
    MidNamMasterDeviceNamesList(const MidNamMasterDeviceNamesList& m);
    ~MidNamMasterDeviceNamesList();
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidNamMasterDeviceNamesList::iterator iMidNamMasterDeviceNamesList;
typedef MidNamMasterDeviceNamesList::const_iterator ciMidNamMasterDeviceNamesList;


//-----------------------------------------------------------------


// T is a pointer to one of the reference lists, such as MidNamNoteNameList.
template<class T, class Compare = std::less<T>, class Alloc = std::allocator<T> > 
  class MidNamReferenceList_t : public std::set<T, Compare, Alloc >
{
  private:
    typedef std::set<T, Compare, Alloc> vlist;

  public:
    typedef typename vlist::iterator iMidNamReferenceList_t;
    typedef typename vlist::const_iterator ciMidNamReferenceList_t;
    typedef std::pair <iMidNamReferenceList_t, bool> MidNamReferenceListPair_t;

    bool add(const T t)
    {
      // Make sure it has a name!
      if(t->name().isEmpty())
        return false;
      MidNamReferenceListPair_t pr = vlist::insert(t);
      return pr.second;
    }
};

typedef MidNamReferenceList_t<MidNamNoteNameList*>   MidNamNoteNameListRefs_t;
typedef MidNamReferenceList_t<MidiNamValNames*>      MidiNamValNamesRefs_t;
typedef MidNamReferenceList_t<MidiNamCtrls*>         MidiNamCtrlsRefs_t;
typedef MidNamReferenceList_t<MidiNamPatchNameList*> MidiNamPatchNameListRefs_t;
typedef MidNamReferenceList_t<MidNamDeviceMode*>     MidNamDeviceModeRefs_t;
typedef MidNamReferenceList_t<MidiNamChannelNameSetAssign*> MidiNamChannelNameSetAssignRefs_t;
typedef MidNamReferenceList_t<MidNamChannelNameSet*> MidNamChannelNameSetRefs_t;

struct MidNamReferencesList
{
  MidNamNoteNameListRefs_t   noteNameListObjs;
  MidiNamValNamesRefs_t      valNamesObjs;
  MidiNamCtrlsRefs_t         ctrlsObjs;
  MidiNamPatchNameListRefs_t patchNameListObjs;
  MidNamDeviceModeRefs_t     deviceModeObjs;
  MidiNamChannelNameSetAssignRefs_t channelNameSetAssignObjs;
  MidNamChannelNameSetRefs_t channelNameSetObjs;

  bool resolveReferences() const;
};

//-----------------------------------------------------------------


class MidNamMIDINameDocument
{
  private:
    MidNamAuthor _author;
    MidNamMasterDeviceNamesList _masterDeviceNamesList;
    MidNamExtendingDeviceNamesList _extendingDeviceNamesList;
    MidNamDeviceModeList _standardDeviceModeList;

  public:
    MidNamMIDINameDocument() {}
    void clear() { 
      _author.clear();
      _masterDeviceNamesList.clear();
      _extendingDeviceNamesList.clear();
      _standardDeviceModeList.clear();
    }
    bool gatherReferences(MidNamReferencesList* refs) const;
    // This gathers all references and objects and resolves the references.
    // Run this AFTER the document has been fully read.
    bool resolveReferences();
    MidNamAuthor& author() { return _author; }
    MidNamMasterDeviceNamesList& masterDeviceNamesList() { return _masterDeviceNamesList; }
    MidNamExtendingDeviceNamesList& extendingDeviceNamesList() { return _extendingDeviceNamesList; }
    MidNamDeviceModeList& standardDeviceModeList() { return _standardDeviceModeList; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamMIDIName
{
  private:
    MidNamMIDINameDocument _MIDINameDocument;
    // Whether the document contains anything
    //  ie MIDINameDocument tag was found and loaded OK.
    bool _isEmpty;

  public:
    MidNamMIDIName() : _isEmpty(true) {}
    bool isEmpty () const { return _isEmpty; }
    void clear() { _MIDINameDocument.clear(); _isEmpty = true; }
    // This gathers all references and objects and resolves the references.
    // Run this AFTER the document has been fully read.
    bool resolveReferences() { return _MIDINameDocument.resolveReferences(); }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    // Like find but if bank high or low are valid,
    //  it searches for a match using them.
    // Whereas find searches for an exact match.
    const MidiNamPatch* findPatch(int channel, int patch) const;

    const MidiNamPatchBankList* getPatchBanks(int channel) const;

    // Find the list of controllers for a channel and/or patch.
    // If channel is -1 or patch is don't care, it looks for defaults.
    const MidiControllerList* getControllers(int channel = -1, int patch = CTRL_PROGRAM_VAL_DONT_CARE) const;

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamMIDINameDocumentList : public std::list<MidNamMIDINameDocument>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};
typedef MidNamMIDINameDocumentList::iterator iMidNamMIDINameDocumentList;
typedef MidNamMIDINameDocumentList::const_iterator ciMidNamMIDINameDocumentList;


//-----------------------------------------------------------------


} // namespace MusECore

#endif
