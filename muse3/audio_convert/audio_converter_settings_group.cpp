//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings_group.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 A T sourceforge D O T net)
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

//#include <stdio.h>

// #include <QDialog>
// // #include <QWidget>
// #include <QListWidgetItem>
// // #include <QVariant>
// // #include <qtextstream.h>

#include "audio_converter_settings_group.h"
#include "audio_convert/audio_converter_plugin.h"

#include "xml.h"

#include "operations.h"
#include "song.h"
#include "track.h"
#include "part.h"
#include "event.h"

#define ERROR_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)

// REMOVE Tim. samplerate. Enabled.
// For debugging output: Uncomment the fprintf section.
#define DEBUG_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)


namespace MusEGlobal {
// This global variable is a pointer so that we can replace it quickly with a new one in RT operations.
MusECore::AudioConverterSettingsGroup* defaultAudioConverterSettings;

void modifyDefaultAudioConverterSettingsOperation(MusECore::AudioConverterSettingsGroup* settings, 
                                                        MusECore::PendingOperationList& ops)
{
  // First, schedule the change to the actual default settings pointer variable.
  ops.add(MusECore::PendingOperationItem(settings, MusECore::PendingOperationItem::ModifyDefaultAudioConverterSettings));
  
  // Now, schedule changes to each wave event if necessary.
  // Note that at this point the above default change has not occurred yet, 
  //  so we must tell it to use what the settings WILL BE, not what they are now.
  for(MusECore::iWaveTrack it = MusEGlobal::song->waves()->begin(); it != MusEGlobal::song->waves()->end(); ++it)
  {
    MusECore::WaveTrack* wtrack = *it;
    for(MusECore::iPart ip = wtrack->parts()->begin(); ip != wtrack->parts()->end(); ++ip)
    {
      MusECore::Part* part = ip->second;
      for(MusECore::iEvent ie = part->nonconst_events().begin(); ie != part->nonconst_events().end(); ++ie)
      {
        MusECore::Event e = ie->second;
        if(e.type() != MusECore::Wave)
          continue;
        if(MusECore::AudioConverterSettingsGroup* cur_ev_settings = e.sndFile().audioConverterSettings())
        {
          // Is the event using its own local settings? Ignore.
          if(cur_ev_settings->useSettings())
            continue;
          e.sndFile().modifyAudioConverterOperation(defaultAudioConverterSettings, false, ops); // false = Default, non-local settings.
        }
      }
    }
  }
}

} // namespace MusEGlobal

namespace MusECore {

// Some hard-coded defaults.
// const int AudioConverterSettingsGroup::defaultPreferredResampler  = 1001;   // SRC resampler.
// const int AudioConverterSettingsGroup::defaultPreferredShifter    = 1003;   // Rubberband stretcher.
// const int AudioConverterSettingsGroup::defaultPreferredLocalValue = -1;     // Use global defaults.
// const AudioConverterSettingsGroupOptions AudioConverterSettingsGroup::defaultOptions(
//   false, 1001, 1003);
const AudioConverterSettingsGroupOptions AudioConverterSettingsGroupOptions::defaultOptions(
  false, 1001, 1003);


void AudioConverterSettingsGroupOptions::write(int level, Xml& xml) const
      {
      //xml.tag(level++, "settings mode=\"%d\" useSettings=\"%d\"", _mode, _useSettings);
      //xml.tag(level++, "settings mode=\"%d\"", _mode);
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
                        //if (tag == "mode")
                        //      _mode = xml.s2().toInt();
                              //_mode = xml.s2();
                        //else 
                        //  if (tag == "useSettings")
                        //      _useSettings = xml.s2().toInt();
                        //else
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


// AudioConverterSettingsGroup::AudioConverterSettingsGroup(
//   AudioConverterPluginList* /*plugList*/, bool /*isLocal*/)
// {
//   //populate(plugList, isLocal);
// }
  
AudioConverterSettingsGroup::~AudioConverterSettingsGroup()
{
  for(iterator i = begin(); i != end(); ++i)
    delete (*i);
}
 
void AudioConverterSettingsGroup::assign(const AudioConverterSettingsGroup& other)
{
  clearDelete();
  for(ciAudioConverterSettingsI i = other.begin(); i != other.end(); ++i)
  {
    AudioConverterSettingsI* other_settingsI = *i;
    AudioConverterSettingsI* new_settingsI = new AudioConverterSettingsI();
    new_settingsI->assign(*other_settingsI);
    push_back(new_settingsI);
  }
//   preferredResampler = other.preferredResampler;
//   preferredShifter = other.preferredShifter;
  _options._useSettings = other._options._useSettings;
  _options._preferredResampler = other._options._preferredResampler;
  _options._preferredShifter = other._options._preferredShifter;
  _isLocal = other._isLocal;
}

// AudioConverterSettingsGroup* AudioConverterSettingsGroup::createSettings(
//   AudioConverterPluginList* plugList, bool isLocal)
// {
//   AudioConverterSettingsGroup* group = new AudioConverterSettingsGroup(plugList, isLocal);
//   return group;
// }

void AudioConverterSettingsGroup::clearDelete()
{
  for(iterator i = begin(); i != end(); ++i)
    delete (*i);
  clear();
}
 
void AudioConverterSettingsGroup::populate(AudioConverterPluginList* plugList, bool isLocal)
{
  clearDelete();
  
  _isLocal = isLocal;
//   if(_isLocal) 
//     _options._preferredShifter = _options._preferredResampler = -1;
//   else
//   {
//     preferredResampler = 1001; // Start with SRC resampler.
//     preferredShifter = 1003;   // Start with rubberband stretcher.
//   }
  
//   _options._useSettings = defaultOptions._useSettings;
//   _options._preferredResampler = defaultOptions._preferredResampler;
//   _options._preferredShifter = defaultOptions._preferredShifter;
//   _options._useSettings = _isLocal ? AudioConverterSettingsGroupOptions::defaultOptions._useSettings : true; // Force non-local to use settings.
//   _options._preferredResampler = AudioConverterSettingsGroupOptions::defaultOptions._preferredResampler;
//   _options._preferredShifter = AudioConverterSettingsGroupOptions::defaultOptions._preferredShifter;
  initOptions();
  
  for(iAudioConverterPlugin ip = plugList->begin(); ip != plugList->end(); ++ip)
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
 
// void AudioConverterSettingsGroup::initOptions(bool isLocal)
// {
//   clearDelete();
//   for(iAudioConverterPlugin ip = MusEGlobal::audioConverterPluginList.begin(); 
//       ip != MusEGlobal::audioConverterPluginList.end(); ++ip)
//   {
//     AudioConverterPlugin* p = *ip;
//     //p->
//   }
// }
  
AudioConverterSettingsI* AudioConverterSettingsGroup::find(int ID) const
//AudioConverterSettingsI* AudioConverterSettingsGroup::find(const char* name, int ID) const
{
  for(const_iterator i = begin(); i != end(); ++i)
  {
    AudioConverterSettingsI* settings = *i;
    if(settings->pluginID() == ID)
      return settings;
  }
  return 0;


  
//   const bool id_valid = (ID != -1);
//   //const bool caps_valid = (capabilities != -1);
//   //AudioConverterPlugin* cap_res = NULL;
//   for(iterator i = begin(); i != end(); ++i)
//   {
//     AudioConverterSettingsI* setI = *i;
//     const bool name_match = (name && (strcmp(name, setI->name().toLatin1().constData()) == 0));
//     const bool ID_match = (id_valid && ID == setI->id());
//     //const bool caps_match = (caps_valid && (plugin->capabilities() & capabilities) == capabilities);
//     if((name && id_valid && name_match && ID_match) || 
//        name_match || 
//        ID_match)
//       return setI;
// //     else if(caps_match)
// //     {
// //       if(!cap_res)
// //         cap_res = plugin;
// //     }
//   }
//   return 0;
  
}

void AudioConverterSettingsGroup::readItem(Xml& xml)
{
      AudioConverterSettingsI* setI = NULL;
      //int id = -1;
      //int mode = -1;
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
//                         if (tag == "id")
//                         {
//                           id = xml.s2().toInt();
//                           if(id >= 0)
//                           {
// // Oops, no, we don't want a new one, it must set an existing one.
// // For example the global defaultAudioConverterSettings is already populated at startup.
// //                             if(AudioConverterPlugin* p = MusEGlobal::audioConverterPluginList.find(NULL, id))
// //                             {
// //                               setI = new AudioConverterSettingsI();
// //                               if(setI->initSettingsInstance(p, _isLocal)) // Returns true if error.
// //                                 delete setI;
// //                               else
// //                               {
// //                                 //setI->read(xml);
// //                                 push_back(setI);
// //                               }
// //                             }
//                             // Settings must already exist ie. from assign() or populate().
//                             setI = find(id);
//                           }
//                         }
//                         else 
                          if (tag == "name")
                          {
                            if(AudioConverterPlugin* p = MusEGlobal::audioConverterPluginList.find(xml.s2().toLatin1().constData()))
                              setI = find(p->id());
                          }
                        
                        //else if (tag == "mode")
                        //      mode = xml.s2().toInt();
                        else
                              fprintf(stderr, "audioConverterSetting unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (tag == "audioConverterSetting")
                        {
//                               //if(id >= 0 && mode >= 0)
//                               if(id >= 0)
//                               {
//                                 if(AudioConverterPlugin* p = MusEGlobal::audioConverterPluginList.find(NULL, id))
//                                 {
//                                   setI = new AudioConverterSettingsI();
//                                   if(setI->initSettingsInstance(p, _isLocal)) // Returns true if error.
//                                     delete setI;
//                                   else
//                                   {
//                                     setI->read(xml);
//                                     push_back(setI);
//                                   }
//                                 }
//                               }
                              return;
                        }
                  default:
                        break;
                  }
            }
}

void AudioConverterSettingsGroup::read(Xml& xml)
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
                                   MusEGlobal::audioConverterPluginList.find(xml.parse1().toLatin1().constData()))
                                _options._preferredResampler = plugin->id();
                        }
                        else if (tag == "preferredShifter")
                        {
                              if(AudioConverterPlugin* plugin = 
                                   MusEGlobal::audioConverterPluginList.find(xml.parse1().toLatin1().constData()))
                              _options._preferredShifter = plugin->id();
                        }
                        else if (tag == "audioConverterSetting")
                              readItem(xml);
//                         else if (tag == "id")
//                         {
//                           const int id = xml.parseInt();
//                           if(id >= 0)
//                           {
//                             if(AudioConverterPlugin* p = MusEGlobal::audioConverterPluginList.find(NULL, id))
//                             {
//                               AudioConverterSettingsI* setI = new AudioConverterSettingsI();
//                               if(setI->initSettingsInstance(p, _isLocal)) // Returns true if error.
//                                 delete setI;
//                               else
//                               {
//                                 setI->read(xml);
//                                 push_back(setI);
//                               }
//                             }
//                           }
//                         }
                        
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

void AudioConverterSettingsGroup::write(int level, Xml& xml) const
{
  if(isDefault())
    return;
  
  xml.tag(level++, "audioConverterSettingsGroup");
  
  if(_options._useSettings != AudioConverterSettingsGroupOptions::defaultOptions._useSettings)
    xml.intTag(level, "useSettings", _options._useSettings);
  
  if(_options._preferredResampler != AudioConverterSettingsGroupOptions::defaultOptions._preferredResampler)
  {
    if(AudioConverterPlugin* plugin = MusEGlobal::audioConverterPluginList.find(NULL, _options._preferredResampler))
      xml.strTag(level, "preferredResampler", plugin->name().toLatin1().constData());
  }
  
  if(_options._preferredShifter != AudioConverterSettingsGroupOptions::defaultOptions._preferredShifter)
  {
    if(AudioConverterPlugin* plugin = MusEGlobal::audioConverterPluginList.find(NULL, _options._preferredShifter))
      xml.strTag(level, "preferredShifter", plugin->name().toLatin1().constData());
  }
  
  for(const_iterator i = begin(); i != end(); ++i)
  {
    if(AudioConverterSettingsI* settings = *i)
      settings->write(level, xml);
  }
  
  xml.tag(--level, "/audioConverterSettingsGroup");
}

// bool AudioConverterSettingsGroup::isSet(int mode) const
// {
//   for(ciAudioConverterSettingsI i = begin(); i != end(); ++i)
//     if((*i)->isSet(mode))
//       return true;
//   return false;
// }

bool AudioConverterSettingsGroup::useSettings(int mode) const
{
//   if(_isLocal)
//   {
//     if(preferredResampler != defaultPreferredLocalValue || preferredShifter != defaultPreferredLocalValue)
//       return true;
//   }
//   else
//   {
//     if(preferredResampler != defaultPreferredResampler || preferredShifter != defaultPreferredShifter)
//       return true;
//   }

  if(_options._useSettings)
    return true;
  for(ciAudioConverterSettingsI i = begin(); i != end(); ++i)
    if((*i)->useSettings(mode))
      return true;
  return false;
}

bool AudioConverterSettingsGroup::isDefault() const
{
  for(const_iterator i = begin(); i != end(); ++i)
  {
    if(AudioConverterSettingsI* settings = *i)
      if(!settings->isDefault())
        return false;
  }
  
  if(!_options.isDefault())
    return false;
  
  return true;
}

// bool AudioConverterSettingsGroup::operator==(const AudioConverterSettingsGroup& other) const
// {
//   return other.preferredResampler == preferredResampler && other.preferredShifter == preferredShifter;
// }

} // namespace MusECore
