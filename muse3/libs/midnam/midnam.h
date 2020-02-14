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

namespace MusECore {

struct MidNamReferencesList;

class MidiNamMIDICommands : public MPEventList
{
  private:
    bool _isPatchMIDICommands;

  public:
    MidiNamMIDICommands() : _isPatchMIDICommands(false) { }
    bool isPatchMIDICommands() const { return _isPatchMIDICommands; }
    void setIsPatchMIDICommands(bool v) { _isPatchMIDICommands = v; }
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


class MidiNamAvailableForChannels : public std::set<MidiNamAvailableChannel>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamAvailableForChannels::iterator iMidiNamAvailableForChannels;
typedef MidiNamAvailableForChannels::const_iterator ciMidiNamAvailableForChannels;
typedef std::pair<iMidiNamAvailableForChannels, bool> MidiNamAvailableForChannelsPair;


//-------------------------------------------------------------------


class MidNamChannelNameSet;
class MidiNamChannelNameSetAssign
{
  private:
    int _channel;
    QString _nameSet;

    // Points to a reference.
    MidNamChannelNameSet* _p_ref;

  public:
    MidiNamChannelNameSetAssign() : _channel(0), _p_ref(nullptr) { }
    MidiNamChannelNameSetAssign(int channel, const QString& nameSet) :
      _channel(channel), _nameSet(nameSet), _p_ref(nullptr) { }
    int channel() const { return _channel; }
    const QString& nameSet() const { return _nameSet; }
    void setNameSet(const QString& nameSet) { 
        _nameSet = nameSet; }
    MidNamChannelNameSet* channelNameSet() { return  _p_ref; }
    void setChannelNameSet(MidNamChannelNameSet* l) { _p_ref = l; }
    void resetChannelNameSet() { _p_ref = nullptr; }
    bool operator<(const MidiNamChannelNameSetAssign& n) const { return _channel < n._channel; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamChannelNameSetAssignments : public std::set<MidiNamChannelNameSetAssign>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamChannelNameSetAssignments::iterator iMidiNamChannelNameSetAssignments;
typedef MidiNamChannelNameSetAssignments::const_iterator ciMidiNamChannelNameSetAssignments;
typedef std::pair<iMidiNamChannelNameSetAssignments, bool> MidiNamChannelNameSetAssignmentsPair;


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

class MidiNamNotes : public std::set<MidiNamNote>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamNotes::iterator iMidiNamNotes;
typedef MidiNamNotes::const_iterator ciMidiNamNotes;
typedef std::pair<iMidiNamNotes, bool> MidiNamNotesPair;


//-------------------------------------------------------------------


class MidiNamNoteGroup : public std::set<MidiNamNote>
{
  private:
    QString _name;

  public:
    MidiNamNoteGroup() {}
    MidiNamNoteGroup(const QString& name) : _name(name) { }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool operator<(const MidiNamNoteGroup& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamNoteGroup::iterator iMidiNamNoteGroup;
typedef MidiNamNoteGroup::const_iterator ciMidiNamNoteGroup;
typedef std::pair<iMidiNamNoteGroup, bool> MidiNamNoteGroupPair;

class MidiNamNoteGroups : public std::set<MidiNamNoteGroup>
{
  public:
  void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamNoteGroups::iterator iMidiNamNoteGroups;
typedef MidiNamNoteGroups::const_iterator ciMidiNamNoteGroups;
typedef std::pair<iMidiNamNoteGroups, bool> MidiNamNoteGroupsPair;


typedef std::map<int /* note */, iMidiNamNotes, std::less<int> > NoteIndexMap;
typedef NoteIndexMap::iterator iNoteIndexMap;
typedef NoteIndexMap::const_iterator ciNoteIndexMap;
typedef std::pair<int /* note */, iMidiNamNotes> NoteIndexMapPair;
typedef std::pair<iNoteIndexMap, iNoteIndexMap> NoteIndexMapRange;


class MidNamNoteNameList
{
  private:
    QString _name;
    MidiNamNoteGroups _noteGroups;
    // Place where ungrouped notes go.
    MidiNamNotes _noteList;
    // Handy reverse lookup map, including groups, indexed by note number.
    NoteIndexMap _noteIndex;
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
    MidNamNoteNameList* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidNamNoteNameList* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool empty() const { return _noteGroups.empty() && _noteList.empty(); }
    const MidiNamNoteGroups& noteGroups() const { return _noteGroups; }
    const MidiNamNotes& noteList() const { return _noteList; }
    const NoteIndexMap& noteIndexMap() const { return _noteIndex; };
    bool addNoteGroup(const MidiNamNoteGroup& group);
    bool addNote(const MidiNamNote& note);
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    bool operator<(const MidNamNoteNameList& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
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

class MidiNamValNames : public std::set<MidiNamVal>
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
    MidiNamValNames* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidiNamValNames* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
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
typedef std::pair<iMidiNamValNames, bool> MidiNamValNamesPair;


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
    MidiNamValNames& valueNames() { return _valueNames; }
    bool empty() const { return _valueNames.empty(); }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};


//-------------------------------------------------------


class MidiNamCtrl
{
  public:
    enum Type { SevenBit=0, FourteenBit, RPN, NRPN };

  private:
    Type _type;
    int _number;
    QString _name;
    MidiNamValues _values;

  public:
    MidiNamCtrl() : _type(SevenBit), _number(0) {}
    MidiNamCtrl(Type type, int number, const QString& name) :
                _type(type), _number(number), _name(name) {}
    bool operator<(const MidiNamCtrl& n) const { return _number < n._number; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamCtrls : public std::set<MidiNamCtrl>
{
  private:
    QString _name;
    // Points to a reference.
    MidiNamCtrls* _p_ref;
    bool _isReference;

  public:
    MidiNamCtrls() : _p_ref(nullptr), _isReference(false) {}
    MidiNamCtrls(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false) { }
    MidiNamCtrls* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidiNamCtrls* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamCtrls::iterator iMidiNamCtrl;
typedef MidiNamCtrls::const_iterator ciMidiNamCtrl;
typedef std::pair<iMidiNamCtrl, bool> MidiNamCtrlPair;


//-----------------------------------------------------------------


class MidiNamPatch
{
  private:
    QString _number;
    QString _name;
    int _programChange;

    MidiNamMIDICommands _patchMIDICommands;
    MidiNamChannelNameSetAssignments _channelNameSetAssignments;

    MidNamNoteNameList _noteNameList;
    MidiNamCtrls _controlNameList;
    
  public:
    MidiNamPatch() : _programChange(0) {}
    MidiNamPatch(const QString& number, const QString& name, int programChange) :
                _number(number), _name(name), _programChange(programChange) {}
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool operator<(const MidiNamPatch& n) const { return _programChange < n._programChange; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamPatchNameList : public std::set<MidiNamPatch>
{
  private:
    QString _name;
    // Points to a reference.
    MidiNamPatchNameList* _p_ref;
    bool _isReference;

  public:
    MidiNamPatchNameList() : _p_ref(nullptr), _isReference(false) {}
    MidiNamPatchNameList(const QString& name) :
      _name(name), _p_ref(nullptr), _isReference(false) { }
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
};
typedef MidiNamPatchNameList::iterator iMidiNamPatchNameList;
typedef MidiNamPatchNameList::const_iterator ciMidiNamPatchNameList;
typedef std::pair<iMidiNamPatchNameList, bool> MidiNamPatchNameListPair;


//-----------------------------------------------------------------


class MidiNamPatchBank
{
  private:
    QString _name;
    bool _ROM;

    MidiNamMIDICommands _MIDICommands;

    MidiNamPatchNameList _patchNameList;

  public:
    MidiNamPatchBank() : _ROM(false) {}
    MidiNamPatchBank(const QString& name, bool ROM) :
                _name(name), _ROM(ROM) {}
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool operator<(const MidiNamPatchBank& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamPatchBankList : public std::set<MidiNamPatchBank>
{
  public:
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamPatchBankList::iterator iMidiNamPatchBankList;
typedef MidiNamPatchBankList::const_iterator ciMidiNamPatchBankList;
typedef std::pair<iMidiNamPatchBankList, bool> MidiNamPatchBankListPair;


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
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool operator<(const MidNamChannelNameSet& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
};

class MidiNamChannelNameSetList : public std::set<MidNamChannelNameSet>
{
  public:
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamChannelNameSetList::iterator iMidiNamChannelNameSetList;
typedef MidiNamChannelNameSetList::const_iterator ciMidiNamChannelNameSetList;
typedef std::pair<iMidiNamChannelNameSetList, bool> MidiNamChannelNameSetListPair;


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
    MidNamDeviceMode* objectOrRef() { return (_isReference && _p_ref) ? _p_ref : this; }
    void setObjectOrRef(MidNamDeviceMode* l) { _p_ref = l; }
    void resetObjectOrRef() { _p_ref = nullptr; }
    bool gatherReferences(MidNamReferencesList* refs) const;
    bool isCustomDeviceMode() const { return _isCustomDeviceMode; }
    void setCustomDeviceMode(bool v) { _isCustomDeviceMode = v; }
    const QString& name() const { return _name; }
    bool isReference() const { return _isReference; }
    void setIsReference(bool v) { _isReference = v; }
    bool operator<(const MidNamDeviceMode& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamDeviceModeList : public std::set<MidNamDeviceMode>
{
  public:
    bool gatherReferences(MidNamReferencesList* refs) const;
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidNamDeviceModeList::iterator iMidNamDeviceModeList;
typedef MidNamDeviceModeList::const_iterator ciMidNamDeviceModeList;
typedef std::pair<iMidNamDeviceModeList, bool> MidNamDeviceModeListPair;


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

class MidiNamModelList : public std::set<MidNamModel>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
};
typedef MidiNamModelList::iterator iMidiNamModelList;
typedef MidiNamModelList::const_iterator ciMidiNamModelList;
typedef std::pair<iMidiNamModelList, bool> MidiNamModelListPair;


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

class MidNamExtendingDeviceNamesList : public std::list<MidNamExtendingDeviceNames>
{
  public:
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
// REMOVE Tim. midnam. Changed.
//     MidNamDeviceModeList& deviceModeList() { return _deviceModeList; }
    const MidNamDeviceModeList* deviceModeList() const { return &_deviceModeList; }
    MidiNamChannelNameSetList& channelNameSetList() { return _channelNameSetList; }
    MidNamNameList& nameList() { return _nameList; }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamMasterDeviceNamesList : public std::list<MidNamMasterDeviceNames>
{
  public:
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

struct MidNamReferencesList
{
  MidNamNoteNameListRefs_t   noteNameListObjs;
  MidiNamValNamesRefs_t      valNamesObjs;
  MidiNamCtrlsRefs_t         ctrlsObjs;
  MidiNamPatchNameListRefs_t patchNameListObjs;
  MidNamDeviceModeRefs_t     deviceModeObjs;

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

    bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const;
};

class MidNamMIDIName
{
  private:
    MidNamMIDINameDocument _MIDINameDocument;

  public:
    MidNamMIDIName() {}
    void clear() { _MIDINameDocument.clear(); }
    // This gathers all references and objects and resolves the references.
    // Run this AFTER the document has been fully read.
    bool resolveReferences() { return _MIDINameDocument.resolveReferences(); }
    void write(int level, MusECore::Xml& xml) const;
    bool read(MusECore::Xml& xml);
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
