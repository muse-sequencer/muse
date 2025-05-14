//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_list.cpp
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
//=========================================================

#include "audio_converter_plugin.h"

#include <QDir>
#include <QString>
#include <QFileInfo>
//#include <QFileInfoList>
#include <QByteArray>

#include <string.h>

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) fprintf(dev, format, ##args)
#define INFO_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

void AudioConverterPluginList::discover(const QString& museGlobalLib, bool debugMsg)
{
  QString s = museGlobalLib + "/converters";

  QDir pluginDir(s);
  if(debugMsg)
  {
    INFO_AUDIOCONVERT(stderr, "searching for audio converters in <%s>\n", s.toLocal8Bit().constData());
  }
  if(pluginDir.exists())
  {
    QFileInfoList list = pluginDir.entryInfoList();
    const QFileInfo* fi;
    for(QFileInfoList::const_iterator it=list.cbegin(); it!=list.cend(); ++it)
    {
      fi = &*it;

      if(!QLibrary::isLibrary(fi->filePath()))
        continue;

      QLibrary qlib;
      qlib.setFileName(fi->filePath());
      // Same as dlopen RTLD_NOW.
      qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
      if(!qlib.load())
      {
        ERROR_AUDIOCONVERT(stderr, "AudioConverterList::discover(): Load library(%s) failed: %s\n",
                           qlib.fileName().toLocal8Bit().constData(), qlib.errorString().toLocal8Bit().constData());
        continue;
      }
      Audio_Converter_Descriptor_Function desc_func =
          (Audio_Converter_Descriptor_Function)qlib.resolve("audio_converter_descriptor");

      if(!desc_func)
      {
        ERROR_AUDIOCONVERT(stderr,
          "Unable to find audio_converter_descriptor() function in plugin "
          "library file \"%s\": %s.\n"
          "Are you sure this is a MusE Audio Converter plugin file?\n",
          qlib.fileName().toLocal8Bit().constData(), qlib.errorString().toLocal8Bit().constData());
        qlib.unload();
        continue;
      }

      const AudioConverterDescriptor* descr;

      for(unsigned long i = 0;; ++i)
      {
        descr = desc_func(i);
        if(!descr)
              break;
        // Make sure it doesn't already exist.
        if(find(descr->_name, descr->_ID))
          continue;
        add(fi, descr);
      }

      qlib.unload();

    }
    if(debugMsg)
    {
      INFO_AUDIOCONVERT(stderr, "%zd Audio converters found\n", size());
    }
  }
}

//---------------------------------------------------------
//   AudioConverterPluginList
//---------------------------------------------------------

AudioConverterPluginList::~AudioConverterPluginList()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginList dtor: Deleting plugins...\\n");
  // Must close the libraries.
  for(const_iterator ip = cbegin(); ip != cend(); ++ip)
    if(*ip)
      delete *ip;
}

void AudioConverterPluginList::clearDelete()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginList clearDelete: Deleting plugins...\n");
  // Must close the libraries.
  for(const_iterator ip = cbegin(); ip != cend(); ++ip)
    if(*ip)
      delete *ip;
  clear();
}

void AudioConverterPluginList::add(const QFileInfo* fi, const AudioConverterDescriptor* d) 
{ 
  push_back(new AudioConverterPlugin(fi, d));
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

AudioConverterPlugin* AudioConverterPluginList::find(const char* name, int ID, int capabilities)
{
  const bool id_valid = (ID != -1);
  const bool caps_valid = (capabilities != -1);
  AudioConverterPlugin* cap_res = nullptr;
  for(const_iterator i = cbegin(); i != cend(); ++i)
  {
    AudioConverterPlugin* plugin = *i;
    const bool name_match = (name && (strcmp(name, plugin->name().toUtf8().constData()) == 0));
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

//---------------------------------------------------------
//   AudioConverterPlugin
//---------------------------------------------------------

AudioConverterPlugin::AudioConverterPlugin(const QFileInfo* f, const AudioConverterDescriptor* d)
{
  fi = *f;
  plugin = nullptr;
  _descriptorFunction = nullptr;
  _references = 0;
  _instNo     = 0;
  _label = QString(d->_label);
  _name = QString(d->_name);
  _uniqueID = d->_ID;
  _maxChannels = d->_maxChannels;
  _capabilities = d->_capabilities;

  _minStretchRatio = d->_minStretchRatio;
  _maxStretchRatio = d->_maxStretchRatio;
  _minSamplerateRatio = d->_minSamplerateRatio;
  _maxSamplerateRatio = d->_maxSamplerateRatio;
  _minPitchShiftRatio = d->_minPitchShiftRatio;
  _maxPitchShiftRatio = d->_maxPitchShiftRatio;
}

AudioConverterPlugin::~AudioConverterPlugin()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin dtor: this:%p plugin:%p id:%d\n", this, plugin, id());

  if(_references > 0)
  {
    DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin dtor: _references is not zero\n");
  }
}

//---------------------------------------------------------
//   reference
//---------------------------------------------------------

bool AudioConverterPlugin::reference()
{
  if(_references == 0)
  {
    _qlib.setFileName(fi.filePath());
    // Same as dlopen RTLD_NOW.
    _qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    if(!_qlib.load())
    {
      ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::reference(): load (%s) failed: %s\n",
        _qlib.fileName().toLocal8Bit().constData(), _qlib.errorString().toLocal8Bit().constData());
      return false;
    }

    Audio_Converter_Descriptor_Function acdf =
      (Audio_Converter_Descriptor_Function)_qlib.resolve("audio_converter_descriptor");
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

  ++_references;

  if(!plugin)
  {
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::reference() Error: %s no plugin!\n",
                       fi.filePath().toLocal8Bit().constData());
    release();
    return false;
  }

  return true;
}

//---------------------------------------------------------
//   release
//---------------------------------------------------------

int AudioConverterPlugin::release()
{
  if(_references == 1)
  {
    // Attempt to unload the library.
    // It will remain loaded if the plugin has shell plugins still in use or there are other references.
    const bool ulres = _qlib.unload();
    // Dummy usage stops unused warnings.
    (void)ulres;
    DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin::release(): No more instances. Result of unloading library %s: %d\n",
      _qlib.fileName().toLocal8Bit().constData(), ulres);

    _descriptorFunction = nullptr;
    plugin = nullptr;
  }
  if(_references > 0)
    --_references;
  return _references;
}

AudioConverterHandle AudioConverterPlugin::instantiate(AudioConverterPluginI* /*plugi*/,
                                                       int systemSampleRate,
                                                       int channels, 
                                                       AudioConverterSettings* settings, 
                                                       AudioConverterSettings::ModeType mode)
{
  if(!reference())
    return nullptr;

  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPlugin::instantiate\n");
  AudioConverterHandle h = plugin->instantiate(systemSampleRate, plugin, channels, settings, mode);
  if(!h)
  {
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPlugin::instantiate() Error: plugin:%s instantiate failed!\n", plugin->_name);
    return nullptr;
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
      // Cleanup each of the converters.
      if(_plugin)
      {
        _plugin->cleanup(handle[i]);
        _plugin->release();
      }
    }
    // Delete the array of converters.
    delete[] handle;
  }
}

void AudioConverterPluginI::init()
{
  _plugin           = nullptr;
  instances         = 0;
  handle            = nullptr;
  _channels         = 0;
}

bool AudioConverterPluginI::initPluginInstance(AudioConverterPlugin* plug,
                                               int systemSampleRate,
                                               int channels, 
                                               AudioConverterSettings* settings, 
                                               AudioConverterSettings::ModeType mode)
{
  if(!plug)
  {
    ERROR_AUDIOCONVERT(stderr, "AudioConverterPluginI::initPluginInstance: Error: plug is zero\n");
    return true;
  }
  
  DEBUG_AUDIOCONVERT(stderr, "AudioConverterPluginI::initPluginInstance: plug:%p id:%d\n", plug, plug->id());
  
  _plugin = plug;
  _channels = channels;
  
  QString inst("-" + QString::number(_plugin->instNo()));
  _name  = _plugin->name() + inst;
  _label = _plugin->label() + inst;

  int max_chans = _plugin->maxChannels(); // -1 = don't care, infinite channels.
  
  if(max_chans > 0)
  {
    instances = _channels / max_chans;
    if(instances < 1)
      instances = 1;
  }
  else
  // Don't care. Unlimited channels.
    instances = 1;
  
  handle = new AudioConverterHandle[instances];
  for(int i = 0; i < instances; ++i)
    handle[i] = nullptr;

  for(int i = 0; i < instances; ++i)
  {
    DEBUG_AUDIOCONVERT(stderr, "  instance:%d\n", i);

    handle[i] = _plugin->instantiate(this, systemSampleRate, _channels, settings, mode);
    if(!handle[i])
      return true;
  }

  return false;
}

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

AudioConverterSettings::ModeType AudioConverterPluginI::mode() const
{
  if(!handle)
    return AudioConverterSettings::RealtimeMode;
  AudioConverterSettings::ModeType fin_m = AudioConverterSettings::RealtimeMode;
  AudioConverterSettings::ModeType m;
  bool first = true;
  for(int i = 0; i < instances; ++i)
    if(handle[i])
    {
      m = handle[i]->mode();
      if(m != fin_m)
      {
        // Check: Each instance should be in the same mode.
        if(!first)
        {
          ERROR_AUDIOCONVERT(stderr,
            "AudioConverterPluginI::mode(): Error: Different mode:%d than first:%d in instance\n", m, fin_m);
        }
        first = false;
        fin_m = m;
      }
    }
  return fin_m;
}

int AudioConverterPluginI::process(
  SNDFILE* sf_handle,
  const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
  const sf_count_t pos,
  float** buffer, const int channels, const int frames, const bool overwrite)
{
  if(!handle) return 0;
  for(int i = 0; i < instances; ++i) 
    if(handle[i])
    {
      int count = handle[i]->process(sf_handle, sf_chans, sf_sr_ratio, sf_stretch_list,
                                     pos, buffer, channels, frames, overwrite);
      // FIXME: Multiple instances point to the SAME SndFile. 
      //        Make separate SndFile instances per-channel,
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
  
  if(_plugin)
  {
    DEBUG_AUDIOCONVERT(stderr, "  _plugin:%p id:%d\n", _plugin, _plugin->id());
    if(_settings)
      _plugin->cleanupSettings(_settings);
    
    _plugin->release();
  }
}

void AudioConverterSettingsI::init()
{
  _plugin    = nullptr;
  _settings  = nullptr;
}

void AudioConverterSettingsI::assign(const AudioConverterSettingsI& other)
{
  _plugin = other._plugin;
  if(!_settings)
  {
    // We are creating an object from the library. Increment the references
    //  so that the library may be opened.
    if(!_plugin->reference())
      return;
    
    _settings = _plugin->createSettings(false); // Don't care about isLocal argument.
    if(!_settings)
    {
      // Give up the reference.
      _plugin->release();
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
  if(!_plugin->reference())
    return true;
  
  _settings = _plugin->createSettings(isLocal);
  if(!_settings)
  {
    // Give up the reference.
    _plugin->release();
    return true;
  }
  
  return false;
}


} // namespace MusECore
