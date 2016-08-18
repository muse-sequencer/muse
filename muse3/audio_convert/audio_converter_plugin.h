//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_list.h
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

#ifndef __AUDIO_CONVERTER_PLUGIN_H__
#define __AUDIO_CONVERTER_PLUGIN_H__

#include <list>
#include <QString>
#include <QFileInfo>

#include "lib_audio_convert/audioconvert.h"

class QWidget;

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   AudioConverterPlugin
//---------------------------------------------------------

class AudioConverterPluginI;
class AudioConverterPlugin {

   protected:
   friend class AudioConverterPluginI;

      void* _handle;
      int _references;
      int _instNo;
      QFileInfo fi;
      Audio_Converter_Descriptor_Function _descriptorFunction;
      const AudioConverterDescriptor* plugin;
      int _uniqueID;
      QString _label;
      QString _name;
      int _maxChannels;
      // Combination of AudioConverter::Capabilities values.
      int _capabilities;
      // Minimum and maximum ratios. -1 means infinite, don't care.
      double _minStretchRatio;
      double _maxStretchRatio;
      double _minSamplerateRatio;
      double _maxSamplerateRatio;
      double _minPitchShiftRatio;
      double _maxPitchShiftRatio;

      // Default settings object created at discovery time, destroyed by us.
//       AudioConverterSettings* _defaultSettings;
      
   public:
      AudioConverterPlugin(QFileInfo* f, const AudioConverterDescriptor* d);
      virtual ~AudioConverterPlugin();
      virtual QString label() const                        { return _label; }
      QString name() const                         { return _name; }
      int id() const                     { return _uniqueID; }
      int maxChannels() const { return _maxChannels; }
      // Combination of AudioConverter::Capabilities values.
      int capabilities() const { return _capabilities; }
      // Minimum and maximum ratios. -1 means infinite, don't care.
      double minStretchRatio() const { return _minStretchRatio; }
      double maxStretchRatio() const { return _maxStretchRatio; }
      double minSamplerateRatio() const { return _minSamplerateRatio; }
      double maxSamplerateRatio() const { return _maxSamplerateRatio; }
      double minPitchShiftRatio() const { return _minPitchShiftRatio; }
      double maxPitchShiftRatio() const { return _maxPitchShiftRatio; }

      QString lib(bool complete = true) const      { return complete ? fi.completeBaseName() : fi.baseName(); }
      QString dirPath(bool complete = true) const  { return complete ? fi.absolutePath() : fi.path(); }
      QString filePath() const                     { return fi.filePath(); }
      QString fileName() const                     { return fi.fileName(); }
        
      int references() const                       { return _references; }
      virtual int incReferences(int);
      int instNo()                                 { return _instNo++;        }

      // Default settings object created at discovery time, destroyed by us.
//       AudioConverterSettings* defaultSettings() { return _defaultSettings; }
      
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      virtual AudioConverterHandle instantiate(AudioConverterPluginI* plugi, 
                                               int channels, 
                                               AudioConverterSettings* settings, 
                                               int mode);
//       virtual void activate(LADSPA_Handle handle) {
//             if (plugin && plugin->activate)
//                   plugin->activate(handle);
//             }
//       virtual void deactivate(LADSPA_Handle handle) {
//             if (plugin && plugin->deactivate)
//                   plugin->deactivate(handle);
//             }
//       virtual void reset(AudioConverterHandle handle) {
//             if(plugin && plugin->cleanup)
//               plugin->reset(handle);
//             }
      virtual void cleanup(AudioConverterHandle handle) {
            if(plugin && plugin->cleanup)
              plugin->cleanup(handle);
            }
      virtual void cleanupSettings(AudioConverterSettings* handle) {
            if(plugin && plugin->cleanupSettings)
              plugin->cleanupSettings(handle);
            }
      // Returns valid object only if plugin is valid ie. reference count is not zero.
      virtual AudioConverterSettings* createSettings(bool isLocal) {
            if(plugin && plugin->createSettings)
              return plugin->createSettings(isLocal);
            else 
              return NULL;
            }
//       virtual void connectPort(LADSPA_Handle handle, unsigned long port, float* value) {
//             if(plugin)
//               plugin->connect_port(handle, port, value);
//             }
//       virtual void apply(LADSPA_Handle handle, unsigned long n) {
//             if(plugin)
//               plugin->run(handle, n);
//             }


      };

//---------------------------------------------------------
//   AudioConverterPluginList
//---------------------------------------------------------

class AudioConverterPluginList : public std::list<AudioConverterPlugin*> {
   public:
      AudioConverterPluginList() { }
      virtual ~AudioConverterPluginList();
      
      // Discover available plugins and fill the list.
      //static void discover();
      void discover();
      
      void add(QFileInfo* fi, const AudioConverterDescriptor* d) 
      { push_back(new AudioConverterPlugin(fi, d)); }

      AudioConverterPlugin* find(const char* name = 0, int ID = -1, int capabilities = -1);
      };

typedef AudioConverterPluginList::iterator iAudioConverterPlugin;
typedef AudioConverterPluginList::const_iterator ciAudioConverterPlugin;
      

//---------------------------------------------------------
//   AudioConverterPluginI
//---------------------------------------------------------

class AudioConverterPluginI {
      AudioConverterPlugin* _plugin;
      int _channels;
      int instances;

      AudioConverterHandle* handle;         // per instance
      QString _name;
      QString _label;
      void init();

   public:
      AudioConverterPluginI();
      virtual ~AudioConverterPluginI();

      AudioConverterPlugin* plugin() const { return _plugin; }

      int pluginID() 
      { return _plugin ? _plugin->id() : -1; }

      // Combination of AudioConverter::Capabilities values.
      int capabilities() const 
      { return _plugin ? _plugin->capabilities() : 0; }
      
      // Minimum and maximum ratios. -1 means infinite, don't care.
      double minStretchRatio() const
      { return _plugin ? _plugin->minStretchRatio() : 1.0; }
      double maxStretchRatio() const
      { return _plugin ? _plugin->maxStretchRatio() : 1.0; }
      double minSamplerateRatio() const
      { return _plugin ? _plugin->minSamplerateRatio() : 1.0; }
      double maxSamplerateRatio() const
      { return _plugin ? _plugin->maxSamplerateRatio() : 1.0; }
      double minPitchShiftRatio() const
      { return _plugin ? _plugin->minPitchShiftRatio() : 1.0; }
      double maxPitchShiftRatio() const
      { return _plugin ? _plugin->maxPitchShiftRatio() : 1.0; }

      //AudioConverterSettings* createSettings(AudioConverterPlugin* plug, bool isLocal = false);
      
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      bool initPluginInstance(AudioConverterPlugin* plug,
                              int channels,
                              AudioConverterSettings* settings,
                              int mode);
      
      // Returns whether all the instances are valid - that each AudioConverter is valid.
      bool isValid() const;
      
      void setChannels(int channels);

      void reset();
      
      sf_count_t seekAudio(SndFile* sf, sf_count_t offset);
      
      int process(SndFile* sf, SNDFILE* sf_handle, sf_count_t pos, float** buffer, 
                 int channels, int frames, bool overwrite);
                            
//       void connect(unsigned long ports, unsigned long offset, float** src, float** dst);
//       void apply(unsigned pos, unsigned long n, unsigned long ports, float** bufIn, float** bufOut);
// 
//       void enableController(unsigned long i, bool v = true)   { controls[i].enCtrl = v; }
//       bool controllerEnabled(unsigned long i) const           { return controls[i].enCtrl; }
//       void enableAllControllers(bool v = true);
// 
//       void activate();
//       void deactivate();
      QString pluginLabel() const    { return _plugin ? _plugin->label() : QString(); }
      QString label() const          { return _label; }
      QString name() const           { return _plugin ? _plugin->name() : QString(); }
      QString lib() const            { return _plugin ? _plugin->lib() : QString(); }
      QString dirPath() const        { return _plugin ? _plugin->dirPath() : QString(); }
      QString fileName() const       { return _plugin ? _plugin->fileName() : QString(); }
//       QString titlePrefix() const;
      };

//---------------------------------------------------------
//   AudioConverterSettingsI
//---------------------------------------------------------

class AudioConverterSettingsI {
      AudioConverterPlugin* _plugin;

      AudioConverterSettings* _settings;
      void init();

   public:
      AudioConverterSettingsI();
      virtual ~AudioConverterSettingsI();

      AudioConverterPlugin* plugin() const { return _plugin; }
      AudioConverterSettings* settings() const { return _settings; }

      int pluginID() 
      { return _plugin ? _plugin->id() : -1; }

      void assign(const AudioConverterSettingsI&);
      bool initSettingsInstance(AudioConverterPlugin* plug, bool isLocal = false);
      
//       // Returns whether any setting is set ie. non-default.
//       // Mode is a combination of AudioConverterSettings::ModeType selecting
//       //  which of the settings to check. Can also be <= 0, meaning all.
//       bool isSet(int mode = -1) const
//       { return _settings ? _settings->isSet(mode) : false; }
      // Returns whether to use these settings or defer to default settings.
      // Mode is a combination of AudioConverterSettings::ModeType selecting
      //  which of the settings to check. Can also be <= 0, meaning all.
      bool useSettings(int mode = -1)
      { return _settings ? _settings->useSettings(mode) : false; }
    
      bool isDefault()
      { return _settings ? _settings->isDefault() : true; } // Act like it's default.
      
      int executeUI(int mode, QWidget* parent = NULL, bool isLocal = false)
      { return _settings ? _settings->executeUI(mode, parent, isLocal) : 0; }
      
      void read(Xml& xml) { if(_settings) _settings->read(xml); }
      void write(int level, Xml& xml) const { if(_settings) _settings->write(level, xml); }
      };
      
} // namespace MusECore


namespace MusEGlobal {
extern MusECore::AudioConverterPluginList audioConverterPluginList;
}


#endif