//=========================================================
//  MusE
//  Linux Music Editor
//
//  midnam.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

namespace MusECore {

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
    void read(MusECore::Xml& xml);
};

class MidiNamNotes : public std::set<MidiNamNote>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamNotes::iterator iMidiNamNotes;
typedef MidiNamNotes::const_iterator ciMidiNamNotes;
typedef std::pair<iMidiNamNotes, bool> MidiNamNotesPair;

class MidiNamNoteGroup : public MidiNamNotes
{
  private:
    QString _name;
    bool _isDefault;

  public:
    MidiNamNoteGroup() : _isDefault(false) {}
    MidiNamNoteGroup(const QString& name, bool isDefault = false) : _name(name), _isDefault(isDefault) { }
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    bool isDefault() const { return _isDefault; }
    void setDefault(bool v) { _isDefault = v; }
    bool operator<(const MidiNamNoteGroup& n) const { return _name < n._name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};

class MidiNamNoteGroups : public std::set<MidiNamNoteGroup>
{
  public:
  void write(int level, MusECore::Xml& xml) const;
  void read(MusECore::Xml& xml);
};
typedef MidiNamNoteGroups::iterator iMidiNamNoteGroups;
typedef MidiNamNoteGroups::const_iterator ciMidiNamNoteGroups;
typedef std::pair<iMidiNamNoteGroups, bool> MidiNamNoteGroupsPair;

class MidiNamNoteNames : public std::map<QString /* name */, MidiNamNoteGroups>
{
  public:
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamNoteNames::iterator iMidiNamNoteNames;
typedef MidiNamNoteNames::const_iterator ciMidiNamNoteNames;
typedef std::pair<QString /* name */, MidiNamNoteGroups> MidiNamNoteNamesPair;
typedef std::pair<iMidiNamNoteNames, iMidiNamNoteNames> MidiNamNoteNamesRange;



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
    void read(MusECore::Xml& xml);
};

class MidiNamValNames : public std::set<MidiNamVal>
{
  private:
    QString _name;

  public:
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamValNames::iterator iMidiNamValNames;
typedef MidiNamValNames::const_iterator ciMidiNamValNames;
typedef std::pair<iMidiNamValNames, bool> MidiNamValNamesPair;

// class MidiNamVals : public std::map<QString /* name */, MidiNamValNames, std::less<QString>>
// {
//   public:
//     void write(int level, MusECore::Xml& xml) const;
//     void read(MusECore::Xml& xml);
// };
// typedef MidiNamVals::iterator iMidiNamVals;
// typedef MidiNamVals::const_iterator ciMidiNamVals;
// typedef std::pair<QString /* name */, MidiNamValNames> MidiNamValsPair;
// typedef std::pair<iMidiNamVals, iMidiNamVals> MidiNamValsRange;


class MidiNamValues
{
  private:
    int _min;
    int _max;
    int _default;
    int _units;
    int _mapping;
    MidiNamValNames _valueNames;
    bool _usesValueNameList;
    QString _valueNameList;

  public:
//     MidiNamValues() : _type(SevenBit), _number(0), _min(0), _max(127), _default(0), _units(0), _mapping(0) {}
//     MidiNamValues(Type type, int number, const QString& name, int min, int max, int def, int units, int mapping) :
//                 _type(type), _number(number), _name(name), _min(min), _max(max),
//                 _default(def), _units(units), _mapping(mapping) {}
    MidiNamValues() : _min(0), _max(127), _default(0), _units(0), _mapping(0), _usesValueNameList(false) {}
    MidiNamValues(int min, int max, int def, int units, int mapping, bool usesValNameList) :
                _min(min), _max(max), _default(def), _units(units),
                _mapping(mapping), _usesValueNameList(usesValNameList) {}
//     bool operator<(const MidiNamValues& n) const { return _number < n._number; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};

// class MidiNamCtrl : public std::set<int /* number */>
class MidiNamCtrl
{
  public:
    enum Type { SevenBit=0, FourteenBit, RPN, NRPN };

  private:
    Type _type;
    int _number;
    QString _name;
//     int _min;
//     int _max;
//     int _default;
//     int _units;
//     int _mapping;
//     //MidiNamVals _vals;
//     QString _namedVals;
    MidiNamValues _values;

  public:
//     MidiNamCtrl() : _type(SevenBit), _number(0), _min(0), _max(127), _default(0), _units(0), _mapping(0) {}
//     MidiNamCtrl(Type type, int number, const QString& name, int min, int max, int def, int units, int mapping) :
//                 _type(type), _number(number), _name(name), _min(min), _max(max),
//                 _default(def), _units(units), _mapping(mapping) {}
    MidiNamCtrl() : _type(SevenBit), _number(0) {}
    MidiNamCtrl(Type type, int number, const QString& name) :
                _type(type), _number(number), _name(name) {}
    bool operator<(const MidiNamCtrl& n) const { return _number < n._number; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
// typedef MidiNamCtrl::iterator iMidiNamCtrl;
// typedef MidiNamCtrl::const_iterator ciMidiNamCtrl;
// typedef std::pair<iMidiNamCtrl, bool> MidiNamCtrlPair;


class MidiNamCtrls : public std::set<MidiNamCtrl>
{
  private:
    QString _name;

  public:
//     MidiNamCtrls() : _type(), _min(), _max(), _default(), _units(), _mapping() {}
//     MidiNamCtrls(Type type, const QString& name, int min, int max, int def, int units, int mapping) :
//                 _type(type), _name(name), _min(min), _max(max), _default(def), _units(units), _mapping(mapping) {}
    const QString& name() const { return _name; }
    void setName(const QString& name) { _name = name; }
    void write(int level, MusECore::Xml& xml) const;
    void read(MusECore::Xml& xml);
};
typedef MidiNamCtrls::iterator iMidiNamCtrl;
typedef MidiNamCtrls::const_iterator ciMidiNamCtrl;
typedef std::pair<iMidiNamCtrl, bool> MidiNamCtrlPair;

} // namespace MusECore

#endif
