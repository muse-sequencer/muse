//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_list.cpp
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
//=========================================================

//#include "lib_audio_convert/audioconvert.h"
#include "audio_converter_plugin.h"

#include "globals.h"
#include "xml.h"
#include "wave.h"

#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QFileInfoList>
#include <QByteArray>

#include <string.h>

#include <dlfcn.h>

#define INFO_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)
#define ERROR_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

// For debugging output: Uncomment the fprintf section.
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

// To make testing keeping the library open easier.
//#define __KEEP_LIBRARY_OPEN__

namespace MusEGlobal {
MusECore::AudioConverterPluginList audioConverterPluginList;
}

namespace MusECore {

void AudioConverterPluginList::discover()
{
  QString s = MusEGlobal::museGlobalLib + "/converters";

  QDir pluginDir(s, QString("*.so"));
  if(MusEGlobal::debugMsg)
  {
        INFO_AUDIOCONVERT(stderr, "searching for audio converters in <%s>\n", s.toLatin1().constData());
  }
  if(pluginDir.exists())
  {
    QFileInfoList list = pluginDir.entryInfoList();
    QFileInfo* fi;
    for(QFileInfoList::iterator it=list.begin(); it!=list.end(); ++it)
    {
      fi = &*it;

      QByteArray ba = fi->filePath().toLatin1();
      const char* path = ba.constData();

      void* handle = dlopen(path, RTLD_NOW);
      if(!handle)
      {
        ERROR_AUDIOCONVERT(stderr, "AudioConverterList::discover(): dlopen(%s) failed: %s\n", path, dlerror());
        continue;
      }
      Audio_Converter_Descriptor_Function desc_func =
          (Audio_Converter_Descriptor_Function)dlsym(handle, "audio_converter_descriptor");

      if(!desc_func)
      {
        const char *txt = dlerror();
        if(txt)
        {
          ERROR_AUDIOCONVERT(stderr,
            "Unable to find audio_converter_descriptor() function in plugin "
            "library file \"%s\": %s.\n"
            "Are you sure this is a MusE Audio Converter plugin file?\n",
            path, txt);
        }
        dlclose(handle);
        continue;
      }
      
      const AudioConverterDescriptor* descr;
      
      for(unsigned long i = 0;; ++i)
      {
        descr = desc_func(i);
        if(!descr)
              break;
        // Make sure it doesn't already exist.
        //if(MusEGlobal::audioConverterPluginList.find(descr->_name, descr->_ID))
        if(find(descr->_name, descr->_ID))
          continue;
        //MusEGlobal::audioConverterPluginList.add(fi, descr);
        add(fi, descr);
      }
        
#ifndef __KEEP_LIBRARY_OPEN__
      // The library must be kept open to use the shared object code.
      dlclose(handle);
#endif
      
    }
    if(MusEGlobal::debugMsg)
    {
      //INFO_AUDIOCONVERT(stderr, "%zd Audio converters found\n", MusEGlobal::audioConverterPluginList.size());
      INFO_AUDIOCONVERT(stderr, "%zd Audio converters found\n", size());
    }
  }
}

//---------------------------------------------------------
//   AudioConverterPluginList
//---------------------------------------------------------

AudioConverterPluginList::~AudioConverterPluginList()
{
//#ifdef __KEEP_LIBRARY_OPEN__
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginList dtor: Closing libraries...\n");
  // Must close the libraries.
  for(iAudioConverterPlugin ip = begin(); ip != end(); ++ip)
    if(*ip)
      delete *ip;
//#endif
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

AudioConverterPlugin* AudioConverterPluginList::find(const char* name, int ID, int capabilities)
{
  const bool id_valid = (ID != -1);
  const bool caps_valid = (capabilities != -1);
  AudioConverterPlugin* cap_res = NULL;
  for(iAudioConverterPlugin i = begin(); i != end(); ++i)
  {
    AudioConverterPlugin* plugin = *i;
    const bool name_match = (name && (strcmp(name, plugin->name().toLatin1().constData()) == 0));
    const bool ID_match = (id_valid && ID == plugin->id());
    const bool caps_match = (caps_valid && (plugin->capabilities() & capabilities) == capabilities);
    if((name && id_valid && name_match && ID_match) || 
       name_match || 
       ID_match)
      return plugin;
    else if(caps_match)
    {
      if(!cap_res)
        cap_res = plugin;
    }
  }
  return cap_res;
}

// void AudioConverterPluginList::readDefaultSettings(Xml& /*xml*/)
// {
// //   int at = 0;
// //   for (;;) {
// //         Xml::Token token = xml.parse();
// //         const QString& tag = xml.s1();
// //         switch (token) {
// //               case Xml::Error:
// //               case Xml::End:
// //                     return 0;
// //               case Xml::TagStart:
// //                     if (tag == "tick")
// //                           tick = xml.parseInt();
// //                     else if (tag == "val")
// //                           tempo = xml.parseInt();
// //                     else
// //                           xml.unknown("TEvent");
// //                     break;
// //               case Xml::Attribut:
// //                     if (tag == "at")
// //                           at = xml.s2().toInt();
// //                     break;
// //               case Xml::TagEnd:
// //                     if (tag == "tempo") {
// //                           return at;
// //                           }
// //               default:
// //                     break;
// //               }
// //         }
// //   return 0;
//   
//   
// //   for (;;) {
// //         Xml::Token token = xml.parse();
// //         const QString& tag = xml.s1();
// //         switch (token) {
// //               case Xml::Error:
// //               case Xml::End:
// //                     return;
// //               case Xml::TagStart:
// //                     if (tag == "preferredResampler")
// //                           preferredResampler = xml.parseInt();
// //                     else if (tag == "preferredShifter")
// //                           preferredShifter = xml.parseInt();
// //                     else if (tag == "SRCSettings")
// //                           SRCResamplerSettings.read(xml);
// //                     else if (tag == "zitaResamplerSettings")
// //                           zitaResamplerSettings.read(xml);
// //                     else if (tag == "rubberbandSettings")
// //                           rubberbandSettings.read(xml);
// //                     //else if (tag == "soundtouchSettings")  // TODO
// //                     //      soundtouchSettings.read(xml);
// //                     
// //                     
// //                     
// //                     
// //                     
// //                     else
// //                           xml.unknown("audioConverterSettings");
// //                     break;
// //               case Xml::Attribut:
// //                         //if (tag == "id")
// //                         //      id = xml.s2().toInt();
// //                         //else
// //                           fprintf(stderr, "audioConverterSettings unknown tag %s\n", tag.toLatin1().constData());
// //                     break;
// //               case Xml::TagEnd:
// //                     if (tag == "audioConverterSettings") {
// //                           return;
// //                           }
// //               default:
// //                     break;
// //               }
// //         }
// 
// }
// 
// void AudioConverterPluginList::writeDefaultSettings(int /*level*/, Xml& /*xml*/) const
// {
// //   xml.tag(level++, "audioConverterSettings");
// //   
// //   for(iAudioConverterPlugin i = begin(); i != end(); ++i)
// //     (*i)->defaultSettings()->write(level, xml);
// //   
// //   xml.tag(--level, "/audioConverterSettings");
// }

//---------------------------------------------------------
//   AudioConverterPlugin
//---------------------------------------------------------

AudioConverterPlugin::AudioConverterPlugin(QFileInfo* f, const AudioConverterDescriptor* d)
{
  fi = *f;
  plugin = NULL;
  _descriptorFunction = NULL;
  _handle = NULL;
  _references = 0;
  _instNo     = 0;
  _label = QString(d->_label);
  _name = QString(d->_name);
  _uniqueID = d->_ID;
//   _maker = QString(d->Maker);
//   _copyright = QString(d->Copyright);
  _maxChannels = d->_maxChannels;
  _capabilities = d->_capabilities;

  _minStretchRatio = d->_minStretchRatio;
  _maxStretchRatio = d->_maxStretchRatio;
  _minSamplerateRatio = d->_minSamplerateRatio;
  _maxSamplerateRatio = d->_maxSamplerateRatio;
  _minPitchShiftRatio = d->_minPitchShiftRatio;
  _maxPitchShiftRatio = d->_maxPitchShiftRatio;

//   _defaultSettings = 0;
//   if(d->createSettings)
//     // Create non-local default settings.
//     _defaultSettings = d->createSettings(false);
}

AudioConverterPlugin::~AudioConverterPlugin()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin dtor: this:%p plugin:%p id:%d\n", this, plugin, id());
  if(plugin)
  {
  //  delete plugin;
    ERROR_AUDIOCONVERT(stderr, "  Error: plugin is not NULL\n");
  }
    
//#ifndef __KEEP_LIBRARY_OPEN__
  if(_handle)
    dlclose(_handle);
//#endif
    
//   if(_defaultSettings)
//     delete _defaultSettings;
}


int AudioConverterPlugin::incReferences(int val)
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin::incReferences: this:%p id:%d _references:%d val:%d\n", this, id(), _references, val);

  int newref = _references + val;

//   if(newref == 0)
  if(newref <= 0)
  {
    _references = 0;
    
#ifndef __KEEP_LIBRARY_OPEN__
    if(_handle)
    {
      DEBUG_AUDIOCONVERT(stderr, "  no more instances, closing library\n");
      dlclose(_handle);
    }

    _handle = NULL;
    _descriptorFunction = NULL;
    plugin = NULL;

#endif
    
    return 0;
  }

  if(!_handle)
  {
    _handle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);

    if(!_handle)
    {
      ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::incReferences dlopen(%s) failed: %s\n",
              fi.filePath().toLatin1().constData(), dlerror());
      return 0;
    }

    Audio_Converter_Descriptor_Function acdf = (Audio_Converter_Descriptor_Function)dlsym(_handle, "audio_converter_descriptor");
    if(acdf)
    {
      const AudioConverterDescriptor* descr;
      for(unsigned long i = 0;; ++i)
      {
        descr = acdf(i);
        if(!descr)
          break;

        QString label(descr->_label);
        if(label == _label)
        {
          _descriptorFunction = acdf;
          plugin = descr;
          break;
        }
      }
    }

    if(plugin)
    {
      _name = QString(plugin->_name);
      _uniqueID = plugin->_ID;
    }
  }

  if(!plugin)
  {
    dlclose(_handle);
    _handle = NULL;
    _references = 0;
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::incReferences Error: %s no plugin!\n", fi.filePath().toLatin1().constData());
    return 0;
  }

  _references = newref;

  return _references;
}

AudioConverterHandle AudioConverterPlugin::instantiate(AudioConverterPluginI* /*plugi*/, 
                                                       int channels, 
                                                       AudioConverterSettings* settings, 
                                                       int mode)
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin::instantiate\n");
  AudioConverterHandle h = plugin->instantiate(MusEGlobal::sampleRate, plugin, channels, settings, mode);
  if(!h)
  {
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::instantiate() Error: plugin:%s instantiate failed!\n", plugin->_name);
    return 0;
  }

  return h;
}


//---------------------------------------------------------
//   AudioConverterPluginI
//---------------------------------------------------------

AudioConverterPluginI::AudioConverterPluginI()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginI ctor\n");
  init();
} 

AudioConverterPluginI::~AudioConverterPluginI()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginI dtor: this:%p handle:%p _plugin:%p\n", this, handle, _plugin);
  if(handle)
  {
    for(int i = 0; i < instances; ++i)
    {
      // Delete each of the converters.
      //delete handle[i];
      if(_plugin)
        _plugin->cleanup(handle[i]);
    }
    // Delete the array of converters.
    delete[] handle;
  }
  
  if (_plugin)
  {
//     deactivate();
    _plugin->incReferences(-1);
  }
}

void AudioConverterPluginI::init()
{
  _plugin           = NULL;
  instances         = 0;
  handle            = NULL;
  _channels         = 0;
}

bool AudioConverterPluginI::initPluginInstance(AudioConverterPlugin* plug,
                                               int channels, 
                                               AudioConverterSettings* settings, 
                                               int mode)
{
  // If plug is given, set it, otherwise attempt to use whatever is already set.
//   if(plug)
//   {
//     if(plug != _plugin)
//       ERROR_AUDIOCONVERT(stderr, 
//         "AudioConverterPluginI::initPluginInstance: Warning: plug:%p != _plugin:%p\n", plug, _plugin);
//     _plugin = plug;
//   }

  //if(!_plugin)
  if(!plug)
  {
    //ERROR_AUDIOCONVERT(stderr, "AudioConverterPluginI::initPluginInstance: Error: No plugin\n");
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPluginI::initPluginInstance: Error: plug is zero\n");
    return true;
  }
  
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginI::initPluginInstance: plug:%p id:%d\n", plug, plug->id());
  
  _plugin = plug;
  _channels = channels;
  
  // We are creating an object from the library. Increment the references
  //  so that the library may be opened.
  if(_plugin->incReferences(1) == 0)
    return true;


  QString inst("-" + QString::number(_plugin->instNo()));
  _name  = _plugin->name() + inst;
  _label = _plugin->label() + inst;

//       unsigned long ins = plug->inports();
//       unsigned long outs = plug->outports();
  int max_chans = _plugin->maxChannels(); // -1 = don't care, infinite channels.
//       if(outs)
  
//       if(max_chans == 0)
//         return true;
  
  if(max_chans > 0)
  {
//         instances = channel / outs;
    instances = _channels / max_chans;
    if(instances < 1)
      instances = 1;
//       }
//       else
//       if(ins)
//       {
//         instances = channel / ins;
//         if(instances < 1)
//           instances = 1;
  }
  else
  // Don't care. Unlimited channels.
    instances = 1;
  
  handle = new AudioConverterHandle[instances];
  for(int i = 0; i < instances; ++i)
    handle[i] = NULL;

  for(int i = 0; i < instances; ++i)
  {
    DEBUG_AUDIOCONVERT(stderr, "  instance:%d\n", i);

    handle[i] = _plugin->instantiate(this, _channels, settings, mode);
    if(!handle[i])
      return true;
  }

//       activate();
  return false;
}

// AudioConverterSettings* AudioConverterPluginI::createSettings(AudioConverterPlugin* plug, bool isLocal)
// { 
//   // If plug is given, set it, otherwise attempt to use whatever is already set.
//   if(plug)
//   {
//     if(plug != _plugin)
//       ERROR_AUDIOCONVERT(stderr, 
//         "AudioConverterPluginI::createSettings: Warning: plug:%p != _plugin:%p\n", plug, _plugin);
//     _plugin = plug;
//   }
//   
//   if(!_plugin)
//   {
//     ERROR_AUDIOCONVERT(stderr, "AudioConverterPluginI::createSettings: No plugin\n");
//     return;
//   }
//   
//   // We are creating an object from the library. Increment the references
//   //  so that the library may be opened.
//   if(_plugin->incReferences(1) == 0)
//     return 0;
//   
//   return _plugin->createSettings(isLocal); 
// }

bool AudioConverterPluginI::isValid() const
{
  if(!handle)
    return false;
  for(int i = 0; i < instances; ++i)
    if(!handle[i] || !handle[i]->isValid())
      return false;
  return true;
}

void AudioConverterPluginI::setChannels(int channels)
{
  if(!handle) return;
  for(int i = 0; i < instances; ++i) 
    if(handle[i])
    {
      // FIXME: Multiple instances point to the SAME SndFile. 
      //        Make seperate SndFile instances per-channel,
      //         make this function per-handle.
      //        Finish this. For now just return after the first handle found:
      handle[i]->setChannels(channels);
      return;
    }
}

void AudioConverterPluginI::reset()
{ 
  if(!handle) return;
  for(int i = 0; i < instances; ++i) 
    if(handle[i])
      handle[i]->reset();
}

// The offset is the offset into the sound file and is NOT converted.
sf_count_t AudioConverterPluginI::seekAudio(SndFile* sf, sf_count_t frame, int offset)
{ 
  if(!handle) return 0;
  for(int i = 0; i < instances; ++i) 
    if(handle[i])
    {
      sf_count_t count = handle[i]->seekAudio(sf, frame, offset);
      // FIXME: Multiple instances point to the SAME SndFile. 
      //        Make seperate SndFile instances per-channel,
      //         make this function per-handle, and work with 
      //         each count returned.
      //        No choice, for now just return after the first handle found:
      return count;
    }
  return 0;
}

int AudioConverterPluginI::process(SndFile* sf, SNDFILE* sf_handle, sf_count_t pos, float** buffer, 
            int channels, int frames, bool overwrite)
{
  if(!handle) return 0;
  for(int i = 0; i < instances; ++i) 
    if(handle[i])
    {
      int count = handle[i]->process(sf, sf_handle, pos, buffer, channels, frames, overwrite);
      // FIXME: Multiple instances point to the SAME SndFile. 
      //        Make seperate SndFile instances per-channel,
      //         make this function per-handle, and work with 
      //         each count returned.
      //        No choice, for now just return after the first handle found:
      return count;
    }
  return 0;
}


//---------------------------------------------------------
//   AudioConverterSettingsI
//---------------------------------------------------------

AudioConverterSettingsI::AudioConverterSettingsI()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterSettingsI ctor\n");
  init();
}

AudioConverterSettingsI::~AudioConverterSettingsI()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterSettingsI dtor: this:%p\n", this);
//   if(handle)
//   {
//     for(int i = 0; i < instances; ++i)
//     {
//       // Delete each of the converters.
//       //delete handle[i];
//       if(_plugin)
//         _plugin->cleanup(handle[i]);
//     }
//     // Delete the array of converters.
//     delete[] handle;
//   }
  
  if(_plugin)
  {
    DEBUG_AUDIOCONVERT(stderr, "  _plugin:%p id:%d\n", _plugin, _plugin->id());
//     deactivate();
    if(_settings)
      _plugin->cleanupSettings(_settings);
    
    _plugin->incReferences(-1);
  }
}

void AudioConverterSettingsI::init()
{
  _plugin           = NULL;
//   instances         = 0;
  _settings            = NULL;
//   _channels         = 0;
}

void AudioConverterSettingsI::assign(const AudioConverterSettingsI& other)
{
  _plugin = other._plugin;
  if(!_settings)
  {
    // We are creating an object from the library. Increment the references
    //  so that the library may be opened.
    if(_plugin->incReferences(1) == 0)
      return;
    
    _settings = _plugin->createSettings(false); // Don't care about isLocal argument.
    if(!_settings)
    {
      // Give up the reference.
      _plugin->incReferences(-1);
      return;
    }
  }
  _settings->assign(*other._settings);
}

bool AudioConverterSettingsI::initSettingsInstance(AudioConverterPlugin* plug, bool isLocal)
{ 
  if(!plug)
  {
    ERROR_AUDIOCONVERT(stderr, "AudioConverterSettingsI::createSettings: Error: plug is zero\n");
    return true;
  }
  
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterSettingsI::initSettingsInstance: this:%p plug:%p id:%d\n", this, plug, plug->id());
  
  _plugin = plug;
    
  // We are creating an object from the library. Increment the references
  //  so that the library may be opened.
  if(_plugin->incReferences(1) == 0)
    return true;
  
  _settings = _plugin->createSettings(isLocal);
  if(!_settings)
  {
    // Give up the reference.
    _plugin->incReferences(-1);
    return true;
  }
  
  return false;
}


} // namespace MusECore
