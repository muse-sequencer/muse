//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings_group.cpp
//  (C) Copyright 2010-2020 Tim E. Real (terminator356 A T sourceforge D O T net)
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
//
//=========================================================

#include "audio_converter_settings_group.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

// Some hard-coded defaults.
const AudioConverterSettingsGroupOptions AudioConverterSettingsGroupOptions::defaultOptions(
  false, 1001, 1003);

void AudioConverterSettingsGroupOptions::write(int level, Xml& xml) const
      {
      xml.tag(level++, "settings\"%d\"");
      
      xml.intTag(level, "useSettings", _useSettings);
      xml.intTag(level, "preferredResampler", _preferredResampler);
      xml.intTag(level, "preferredShifter", _preferredShifter);
      
      xml.tag(--level, "/settings");
      
      }

void AudioConverterSettingsGroupOptions::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "useSettings")
                              _useSettings = xml.parseInt();
                        else 
                          if (tag == "preferredResampler")
                              _preferredResampler = xml.parseInt();
                        else 
                          if (tag == "preferredShifter")
                              _preferredShifter = xml.parseInt();
                        else
                              xml.unknown("settings");
                        break;
                  case Xml::Attribut:
                              fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (tag == "settings") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }


AudioConverterSettingsGroup::~AudioConverterSettingsGroup()
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    delete (*i);
}
 
void AudioConverterSettingsGroup::assign(const AudioConverterSettingsGroup& other)
{
  clearDelete();
  for(const_iterator i = other.cbegin(); i != other.cend(); ++i)
  {
    AudioConverterSettingsI* other_settingsI = *i;
    AudioConverterSettingsI* new_settingsI = new AudioConverterSettingsI();
    new_settingsI->assign(*other_settingsI);
    push_back(new_settingsI);
  }
  _options._useSettings = other._options._useSettings;
  _options._preferredResampler = other._options._preferredResampler;
  _options._preferredShifter = other._options._preferredShifter;
  _isLocal = other._isLocal;
}

void AudioConverterSettingsGroup::clearDelete()
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
    delete (*i);
  clear();
}
 
void AudioConverterSettingsGroup::populate(AudioConverterPluginList* plugList, bool isLocal)
{
  clearDelete();
  
  _isLocal = isLocal;

  initOptions();
  
  for(ciAudioConverterPlugin ip = plugList->cbegin(); ip != plugList->cend(); ++ip)
  {
    if(AudioConverterPlugin* p = *ip)
    {
      AudioConverterSettingsI* setI = new AudioConverterSettingsI();
      if(setI->initSettingsInstance(p, isLocal))
      {
        delete setI;
        continue;
      }
      push_back(setI);
    }
  }
}
 
AudioConverterSettingsI* AudioConverterSettingsGroup::find(int ID) const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    AudioConverterSettingsI* settings = *i;
    if(settings->pluginID() == ID)
      return settings;
  }
  return 0;
}

void AudioConverterSettingsGroup::readItem(Xml& xml, AudioConverterPluginList* plugList)
{
      AudioConverterSettingsI* setI = nullptr;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "settings")
                        {
                          if(setI)
                            setI->read(xml);
                        }
                        else
                              xml.unknown("audioConverterSetting");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                        {
                          if(AudioConverterPlugin* p = plugList->find(xml.s2().toLatin1().constData()))
                            setI = find(p->id());
                        }
                        else
                              fprintf(stderr, "audioConverterSetting unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (tag == "audioConverterSetting")
                        {
                              return;
                        }
                  default:
                        break;
                  }
            }
}

void AudioConverterSettingsGroup::read(Xml& xml, AudioConverterPluginList* plugList)
{
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "useSettings")
                              _options._useSettings = xml.parseInt();
                        else if (tag == "preferredResampler")
                        {
                              if(AudioConverterPlugin* plugin = 
                                   plugList->find(xml.parse1().toLatin1().constData()))
                                _options._preferredResampler = plugin->id();
                        }
                        else if (tag == "preferredShifter")
                        {
                              if(AudioConverterPlugin* plugin = 
                                   plugList->find(xml.parse1().toLatin1().constData()))
                              _options._preferredShifter = plugin->id();
                        }
                        else if (tag == "audioConverterSetting")
                              readItem(xml, plugList);
                        else
                              xml.unknown("audioConverterSettingsGroup");
                        break;
                  case Xml::Attribut:
                              fprintf(stderr, "audioConverterSettingsGroup unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (tag == "audioConverterSettingsGroup") {
                              return;
                              }
                  default:
                        break;
                  }
            }
}

void AudioConverterSettingsGroup::write(int level, Xml& xml, AudioConverterPluginList* plugList) const
{
  if(isDefault())
    return;
  
  xml.tag(level++, "audioConverterSettingsGroup");
  
  if(_options._useSettings != AudioConverterSettingsGroupOptions::defaultOptions._useSettings)
    xml.intTag(level, "useSettings", _options._useSettings);
  
  if(_options._preferredResampler != AudioConverterSettingsGroupOptions::defaultOptions._preferredResampler)
  {
    if(AudioConverterPlugin* plugin = plugList->find(nullptr, _options._preferredResampler))
      xml.strTag(level, "preferredResampler", plugin->name().toLatin1().constData());
  }
  
  if(_options._preferredShifter != AudioConverterSettingsGroupOptions::defaultOptions._preferredShifter)
  {
    if(AudioConverterPlugin* plugin = plugList->find(nullptr, _options._preferredShifter))
      xml.strTag(level, "preferredShifter", plugin->name().toLatin1().constData());
  }
  
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    if(AudioConverterSettingsI* settings = *i)
      settings->write(level, xml);
  }
  
  xml.tag(--level, "/audioConverterSettingsGroup");
}

bool AudioConverterSettingsGroup::useSettings(int mode) const
{
  if(_options._useSettings)
    return true;
  for(const_iterator i = cbegin(); i != cend(); ++i)
    if((*i)->useSettings(mode))
      return true;
  return false;
}

bool AudioConverterSettingsGroup::isDefault() const
{
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    if(AudioConverterSettingsI* settings = *i)
      if(!settings->isDefault())
        return false;
  }
  
  if(!_options.isDefault())
    return false;
  
  return true;
}

} // namespace MusECore
