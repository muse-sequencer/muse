//=============================================================================
//  MusE
//  Linux Music Editor
//
//   metronome_class.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __METRONOME_CLASS_H__
#define __METRONOME_CLASS_H__

#include <QString>

#include <vector>
#include <map>
#include <cstdint>

#include "xml.h"

namespace MusECore {

struct MetroAccent
{
  public:
    enum AccentType { NoAccent = 0x0, Accent1 = 0x1, Accent2 = 0x2, AllAccents = Accent1 | Accent2 };
    typedef int AccentTypes_t;

    AccentTypes_t _accentType;

  public:
    MetroAccent() : _accentType(NoAccent) { }

    // Returns true if the accents are the same in both structs.
    bool operator==(const MetroAccent& other) const;
    // Returns true if the accents are NOT the same in both structs.
    bool operator!=(const MetroAccent& other) const;
    // Returns true if the accents are 'off' (NoAccent).
    bool isBlank(AccentTypes_t types = AllAccents) const;
    // Blanks the given type of accent.
    void blank(AccentTypes_t types);
};

class MetroAccents : public std::vector<MetroAccent> 
{
  public:
    // Returns true if all the accents are the same in both lists.
    bool operator==(const MetroAccents& other) const;
    // Returns true if all the accents are 'off'.
    bool isBlank(MetroAccent::AccentTypes_t types = MetroAccent::AllAccents) const;
    // Blanks all accents of a given type.
    void blank(MetroAccent::AccentTypes_t types);
};

class MetroAccentsStruct
{
  private:
    static std::uint64_t _idGen;
    std::uint64_t newId() { return _idGen++; }

    std::uint64_t _id;

  public:
    enum MetroAccentsType { NoType = 0x0, 
      User = 0x1,
      UserPreset = 0x2,
      FactoryPreset = 0x4,
      AllTypes = User | UserPreset | FactoryPreset
    };
    typedef int MetroAccentsTypes_t;

    MetroAccents _accents;
    MetroAccentsType _type;

    std::uint64_t id() const { return _id; }

    // Returns true if all the accents are 'off'.
    bool isBlank(MetroAccent::AccentTypes_t types = MetroAccent::AllAccents) const;
    // Blanks all accents of a given type.
    void blank(MetroAccent::AccentTypes_t types);

    // Assigns the members of the given MetroAccentsStruct to this one, EXCEPT for the ID.
    // Returns this MetroAccentsStruct.
    MetroAccentsStruct& assign(const MetroAccentsStruct&);
    // Creates a copy of this MetroAccentsStruct but with a new ID.
    MetroAccentsStruct copy() const;

    void read(MusECore::Xml&);
    void write(int, MusECore::Xml&) const;

    MetroAccentsStruct(MetroAccentsType type) : _id(newId()), _type(type) { }
};

class MetroAccentsMap : public std::map<const int /*beats*/, MetroAccentsStruct, std::less<int> >
{
  public:
    // Returns beats.
    int read(MusECore::Xml&);
    void write(int, MusECore::Xml&) const;
};

class MetroAccentsPresets : public std::vector<MetroAccentsStruct>
{
  public:
    //// Returns beats.
    //int read(MusECore::Xml&);
    void write(int, MusECore::Xml&, int beats, MetroAccentsStruct::MetroAccentsType type) const;

    iterator find(const MetroAccentsStruct&, const MetroAccentsStruct::MetroAccentsTypes_t& types);
    const_iterator find(const MetroAccentsStruct&, const MetroAccentsStruct::MetroAccentsTypes_t& types) const;
    iterator findId(std::uint64_t id);
    const_iterator findId(std::uint64_t id) const;
};

class MetroAccentsPresetsMap : public std::map<const int /*beats*/, MetroAccentsPresets, std::less<int> >
{
  public:
    void read(MusECore::Xml&);
    void write(int, MusECore::Xml&, MetroAccentsStruct::MetroAccentsType type) const;
    void writeMDF(const QString& filepath, MetroAccentsStruct::MetroAccentsType type) const;
    // Fills the given accents map with the first items found in this presets map.
    void defaultAccents(MetroAccentsMap* accents, MetroAccentsStruct::MetroAccentsType type) const;
};

struct MetronomeSettings
{
  enum ClickSamples {
      origSamples = 0,
      newSamples
  };

  int preMeasures;
  unsigned char measureClickNote;
  unsigned char measureClickVelo;
  unsigned char beatClickNote;
  unsigned char beatClickVelo;
  unsigned char accentClick1;
  unsigned char accentClick1Velo;
  unsigned char accentClick2;
  unsigned char accentClick2Velo;

  unsigned char clickChan;
  unsigned char clickPort;
  bool precountEnableFlag;
  bool precountFromMastertrackFlag;
  int precountSigZ;
  int precountSigN;
  bool precountOnPlay;
  bool precountMuteMetronome;
  bool precountPrerecord;
  bool precountPreroll;
  bool midiClickFlag;
  bool audioClickFlag;
  float audioClickVolume;
  float measClickVolume;
  float beatClickVolume;
  float accent1ClickVolume;
  float accent2ClickVolume;
  ClickSamples clickSamples;
  QString measSample;
  QString beatSample;
  QString accent1Sample;
  QString accent2Sample;

  MusECore::MetroAccentsPresetsMap metroAccentPresets;
  // This is a pointer so that it can quickly swapped with a new map in real-time.
  MusECore::MetroAccentsMap* metroAccentsMap;

  MetronomeSettings();
  ~MetronomeSettings();
};

extern void initMetronomePresets(const QString& dir, MetroAccentsPresetsMap* presetMap, bool debug = false);

} // namespace MusECore

#endif
