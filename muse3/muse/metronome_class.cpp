//=============================================================================
//  MusE
//  Linux Music Editor
//
//   metronome_class.cpp
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

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFile>

#include "metronome_class.h"

namespace MusECore {

// Static.
std::uint64_t MetroAccentsStruct::_idGen = 0;

//---------------------------------------------------------
//  assign
//   Assigns the members of the given MetroAccentsStruct to this one, EXCEPT for the ID.
//   Returns this MetroAccentsStruct.
//---------------------------------------------------------

MetroAccentsStruct& MetroAccentsStruct::assign(const MetroAccentsStruct& m)
{
  _type = m._type;
  _accents = m._accents;
  return *this;
}

//---------------------------------------------------------
//  isBlank
//   Returns true if all the accents are 'off'.
//---------------------------------------------------------
bool MetroAccentsStruct::isBlank(MetroAccent::AccentTypes_t types) const
{
  return _accents.isBlank(types);
}

//---------------------------------------------------------
//  blank
//   Returns true if all the accents are 'off'.
//---------------------------------------------------------
void MetroAccentsStruct::blank(MetroAccent::AccentTypes_t types)
{
  _accents.blank(types);
}

//---------------------------------------------------------
//  copy
//   Creates a copy of this MetroAccentsStruct but with a new ID.
//---------------------------------------------------------

MetroAccentsStruct MetroAccentsStruct::copy() const
{
  return MetroAccentsStruct(_type).assign(*this);
}

//---------------------------------------------------------
//   loadMDF (Metronome Definition File)
//---------------------------------------------------------

static void loadMDF(const QString& filepath, MetroAccentsPresetsMap* presetMap, bool debug = false)
{
  QFile f(filepath);
  if(!f.open(QIODevice::ReadOnly /*| QIODevice::Text*/))
        return;
  if (debug)
        fprintf(stderr, "READ MDF %s\n", filepath.toLatin1().constData());
  Xml xml(&f);

  bool skipmode = true;
  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    f.close();
                    return;
              case Xml::TagStart:
                    if (skipmode && tag == "muse")
                          skipmode = false;
                    else if (skipmode)
                          break;
                    else if (tag == "metroAccPresets") {
                          presetMap->read(xml);
                        }
                    else
                          xml.unknown("muse");
                    break;
              case Xml::Attribut:
                    break;
              case Xml::TagEnd:
                    if (!skipmode && tag == "muse") {
                          f.close();
                          return;
                          }
              default:
                    break;
              }
        }
  f.close();
}

void MetroAccentsPresetsMap::writeMDF(const QString& filepath, MetroAccentsStruct::MetroAccentsType type) const
{
  QFile f(filepath);
  if(!f.open(QIODevice::WriteOnly /*| QIODevice::Text*/))
    return;
  MusECore::Xml xml(&f);

  int level = 0;
  xml.header();
  level = xml.putFileVersion(level);

  write(level, xml, type);

  level--;
  xml.etag(level, "muse");
  f.close();
}

void MetroAccentsPresetsMap::defaultAccents(MetroAccentsMap* accents, MetroAccentsStruct::MetroAccentsType type) const
{
  const_iterator ipm_end = cend();
  for(const_iterator i = cbegin(); i != ipm_end; ++i)
  {
    const int& beats = i->first;
    const MetroAccentsPresets& pre = i->second;
    if(!pre.empty())
    {
      MetroAccentsPresets::const_iterator imap_end = pre.cend();
      for(MetroAccentsPresets::const_iterator imap = pre.cbegin(); imap != imap_end; ++imap)
      {
        MetroAccentsStruct mas = *imap;
        if(mas._type != type)
          continue;
        // Change the type to user.
        mas._type = MetroAccentsStruct::MetroAccentsType::User;
        std::pair<MetroAccentsMap::iterator, bool> res = 
          accents->insert(std::pair<const int, MetroAccentsStruct>(beats, mas));
        if(!res.second)
          res.first->second = mas;
        break;
      }
    }
  }
}

void MetroAccentsPresetsMap::write(int level, MusECore::Xml& xml, MetroAccentsStruct::MetroAccentsType type) const
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    i->second.write(level, xml, i->first, type);
}

void MetroAccentsPresetsMap::read(MusECore::Xml& xml)
{
  bool ok;
  int beats = 0;
  MetroAccentsStruct::MetroAccentsType type = MetroAccentsStruct::NoType;
  for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return;
              case MusECore::Xml::TagStart:
                          xml.unknown("metroAccPresets");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "type")
                    {
                          const int itype = xml.s2().toInt();
                          switch(itype)
                          {
                            case MetroAccentsStruct::UserPreset:
                              type = MetroAccentsStruct::UserPreset;
                            break;

                            case MetroAccentsStruct::FactoryPreset:
                              type = MetroAccentsStruct::FactoryPreset;
                            break;

                            // Don't want these.
                            case MetroAccentsStruct::User:
                            default:
                            break;
                          }
                    }
                    else if (tag == "beats")
                          beats = xml.s2().toInt();
                    break;
              case Xml::Text:
                    {
                      if(type == MetroAccentsStruct::NoType)
                      {
                        fprintf(stderr, "MetroAccentsPresets::read: Unknown type\n");
                        break;
                      }

                      int len = tag.length();
                      int i = 0;
                      for(;;) 
                      {
                        int acctypes = MetroAccent::NoAccent;
                        MetroAccentsStruct mas(type);
                        for(;;) 
                        {
                          while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                            ++i;
                          if(i == len)
                                break;

                          QString fs;
                          while(i < len && tag[i] != ',' && tag[i] != ' ')
                          {
                            fs.append(tag[i]); 
                            ++i;
                          }
                          if(i == len)
                                break;

                          acctypes = fs.toInt(&ok);
                          if(!ok)
                          {
                            fprintf(stderr, "MetroAccentsPresets::read failed reading accent types string: %s\n", fs.toLatin1().constData());
                            break;
                          }

                          MetroAccent ma;
                          ma._accentType = acctypes;
                          mas._accents.push_back(ma);

                          while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                            ++i;
                          if(i == len || tag[i] != ',')
                                break;
                        }

                        // Don't bother reading if there are no accents set.
                        if(beats > 0 && !mas.isBlank())
                        {
                          std::pair<iterator, bool> pm_res = insert(std::pair<const int, MetroAccentsPresets>(beats, MetroAccentsPresets()));
                          iterator ipm = pm_res.first;
                          MetroAccentsPresets& mp = ipm->second;
                          MetroAccentsPresets::iterator imap = 
                            mp.find(mas, MetroAccentsStruct::FactoryPreset | MetroAccentsStruct::UserPreset);
                          if(imap == mp.end())
                            mp.push_back(mas);
                          else
                            // In case of duplicates (an existing found), replace with latest.
                            *imap = mas;
                        }

                        while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                          ++i;
                        if(i == len)
                              break;
                      }
                    }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "metroAccPresets")
                    {
                      return;
                    }
              default:
                    break;
              }
        }
}

MetroAccentsPresets::iterator MetroAccentsPresets::find(
  const MetroAccentsStruct& mas, const MetroAccentsStruct::MetroAccentsTypes_t& types)
{
  iterator iend = end();
  for(iterator i = begin(); i != iend; ++i)
  {
    const MetroAccentsStruct& m = *i;
    const MetroAccentsStruct::MetroAccentsType& m_type = m._type;
    if(m._accents == mas._accents && (types & m_type))
      return i;
  }
  return iend;
}

MetroAccentsPresets::const_iterator MetroAccentsPresets::find(
  const MetroAccentsStruct& mas, const MetroAccentsStruct::MetroAccentsTypes_t& types) const
{
  const_iterator iend = cend();
  for(const_iterator i = cbegin(); i != iend; ++i)
  {
    const MetroAccentsStruct& m = *i;
    const MetroAccentsStruct::MetroAccentsType& m_type = m._type;
    if(m._accents == mas._accents && (types & m_type))
      return i;
  }
  return iend;
}

MetroAccentsPresets::iterator MetroAccentsPresets::findId(std::uint64_t id)
{
  iterator iend = end();
  for(iterator i = begin(); i != iend; ++i)
  {
    if(i->id() == id)
      return i;
  }
  return iend;
}

MetroAccentsPresets::const_iterator MetroAccentsPresets::findId(std::uint64_t id) const
{
  const_iterator iend = cend();
  for(const_iterator i = cbegin(); i != iend; ++i)
  {
    if(i->id() == id)
      return i;
  }
  return iend;
}

void MetroAccentsPresets::write(int level, MusECore::Xml& xml,
  int beats, MetroAccentsStruct::MetroAccentsType type) const
{
  if(empty())
    return;
  // Check if there's anything to write.
  const_iterator imap = cbegin();
  for( ; imap != cend(); ++imap)
  {
    if(imap->_type == type)
      break;
  }
  if(imap == cend())
    return;

  xml.tag(level++, "metroAccPresets type=\"%d\" beats=\"%d\"", type, beats);
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    if(i->_type != type)
      continue;
    i->write(level, xml);
  }
  xml.tag(--level, "/metroAccPresets");
}

void MetroAccentsStruct::write(int level, MusECore::Xml& xml) const
{
  // Don't bother writing if there are no accents set.
  if(isBlank())
    return;
  QString acc_s;
  const int sz_m1 = _accents.size() - 1;
  int count1 = 0;
  int count2 = 0;
  int level_in = 0;
  for(MetroAccents::const_iterator i = _accents.cbegin(); i != _accents.cend(); ++count1, ++i)
  {
    const MetroAccent& acc = *i;
    acc_s += QString::number(acc._accentType);
    if(count1 < sz_m1)
      acc_s += ", ";
    ++count2;
    if(count2 >= 16)
    {
      xml.put(level + level_in, "%s", acc_s.toLatin1().constData());
      if(level_in == 0)
        level_in = 1;
      acc_s.clear();
      count2 = 0;
    }
  }
  if(count2)
    xml.put(level + level_in, "%s", acc_s.toLatin1().constData());
}

void MetroAccentsStruct::read(MusECore::Xml& xml)
{
  bool ok;
  for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return;
              case MusECore::Xml::TagStart:
                          xml.unknown("MetroAccentsStruct");
                    break;
              case MusECore::Xml::Attribut:
                    break;
              case Xml::Text:
                    {
                      int len = tag.length();
                      int acctypes = MetroAccent::NoAccent;

                      int i = 0;
                      for(;;) 
                      {
                            while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                              ++i;
                            if(i == len)
                                  break;

                            QString fs;
                            while(i < len && tag[i] != ',' && tag[i] != ' ')
                            {
                              fs.append(tag[i]); 
                              ++i;
                            }
                            if(i == len)
                                  break;

                            acctypes = fs.toInt(&ok);
                            if(!ok)
                            {
                              fprintf(stderr, "MetroAccentsStruct::read failed reading accent types string: %s\n", fs.toLatin1().constData());
                              break;
                            }

                            MetroAccent ma;
                            ma._accentType = acctypes;
                            _accents.push_back(ma);

                            while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                              ++i;
                            if(i == len || tag[i] != ',')
                                  break;
                      }
                    }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "metroAccents")
                    {
                      return;
                    }
              default:
                    break;
              }
        }
  return;
}

bool MetroAccent::operator==(const MetroAccent& other) const
{
  return _accentType == other._accentType;
}

bool MetroAccent::operator!=(const MetroAccent& other) const
{
  return _accentType != other._accentType;
}

bool MetroAccent::isBlank(AccentTypes_t types) const
{
  return (_accentType & types) == NoAccent;
}

void MetroAccent::blank(AccentTypes_t types)
{
  _accentType &= ~types;
}

bool MetroAccents::operator==(const MetroAccents& other) const
{
  const size_type sz = size();
  if(sz != other.size())
    return false;
  for(size_type i = 0; i < sz; ++i)
  {
    if(at(i) != other.at(i))
      return false;
  }
  return true;
}

bool MetroAccents::isBlank(MetroAccent::AccentTypes_t types) const
{
  const size_type sz = size();
  for(size_type i = 0; i < sz; ++i)
  {
    if(!at(i).isBlank(types))
      return false;
  }
  return true;
}

void MetroAccents::blank(MetroAccent::AccentTypes_t types)
{
  iterator iend = end();
  for(iterator i = begin(); i != iend; ++i)
    i->blank(types);
}

// Returns beats.
int MetroAccentsMap::read(MusECore::Xml& xml)
{
  bool ok;
  int beats = 0;
  for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return 0;
              case MusECore::Xml::TagStart:
                          xml.unknown("MetroAccentsMap");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "beats")
                          beats = xml.s2().toInt();
                    break;
              case Xml::Text:
                    {
                      int len = tag.length();
                      int i = 0;
                      int acctypes = MetroAccent::NoAccent;
                      MetroAccentsStruct mas(MetroAccentsStruct::User);
                      for(;;) 
                      {
                        while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                          ++i;
                        if(i == len)
                              break;

                        QString fs;
                        while(i < len && tag[i] != ',' && tag[i] != ' ')
                        {
                          fs.append(tag[i]); 
                          ++i;
                        }
                        if(i == len)
                              break;

                        acctypes = fs.toInt(&ok);
                        if(!ok)
                        {
                          fprintf(stderr, "MetroAccentsMap::read failed reading accent types string: %s\n", fs.toLatin1().constData());
                          break;
                        }

                        MetroAccent ma;
                        ma._accentType = acctypes;
                        mas._accents.push_back(ma);
                        
                        while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                          ++i;
                        if(i == len || tag[i] != ',')
                              break;
                      }

                      // Don't bother reading if there are no accents set.
                      if(beats > 0 && !mas.isBlank())
                      {
                        std::pair<iterator, bool> res = insert(std::pair<const int, MetroAccentsStruct>(beats, mas));
                        if(!res.second)
                          // In case of duplicate beats (an existing beats entry found), replace with latest.
                          res.first->second = mas;
                      }
                    }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "metroAccMap") {
                          return beats;
                          }
              default:
                    break;
              }
        }
  return 0;
}

void MetroAccentsMap::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    // Don't bother writing if there are no accents set.
    if(i->second.isBlank())
      continue;
    xml.tag(level, "metroAccMap beats=\"%d\"", i->first);
    i->second.write(level + 1, xml);
    xml.tag(level, "/metroAccMap");
  }
}

//---------------------------------------------------------
//   initMetronomePresets
//---------------------------------------------------------

void initMetronomePresets(const QString& dir, MetroAccentsPresetsMap* presetMap, bool debug)
{
  if(!QDir(dir).exists())
  {
    fprintf(stderr, "Metronome directory not found: %s\n", dir.toLatin1().constData());
    return;
  }
    
  if (debug)
    fprintf(stderr, "Load metronome presets from <%s>\n", dir.toLatin1().constData());
  QDirIterator metro_di(dir, QStringList() << "*.mdf", QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
  while(metro_di.hasNext())
    loadMDF(metro_di.next(), presetMap, debug);
}

MetronomeSettings::MetronomeSettings()
{
  preMeasures = 2;
  measureClickNote = 37;
  measureClickVelo = 127;
  beatClickNote    = 42;
  beatClickVelo    = 120;
  accentClick1     = 44;
  accentClick1Velo = 100;
  accentClick2     = 42;
  accentClick2Velo = 100;

  clickChan = 9;
  clickPort = 0;
  precountEnableFlag = false;
  precountFromMastertrackFlag = true;
  precountSigZ = 4;
  precountSigN = 4;
  precountOnPlay = false;
  precountMuteMetronome = false;
  precountPrerecord = false;
  precountPreroll = false;
  midiClickFlag   = false;
  audioClickFlag  = true;
  audioClickVolume = 0.5f;
  measClickVolume = 1.0f;
  beatClickVolume = 1.0f;
  accent1ClickVolume = 0.1f;
  accent2ClickVolume = 0.1f;
  clickSamples = newSamples;
  measSample = QString("klick1.wav");
  beatSample = QString("klick2.wav");
  accent1Sample = QString("klick3.wav");
  accent2Sample = QString("klick4.wav");

  metroAccentsMap = new MetroAccentsMap();
}

MetronomeSettings::~MetronomeSettings()
{
  if(metroAccentsMap)
    delete metroAccentsMap;
  metroAccentsMap = nullptr;
}

} // namespace MusECore
