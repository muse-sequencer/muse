//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.h,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim 
//  (C) Copyright 2009-2016 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#ifndef __AUDIOCONVERT_H__
#define __AUDIOCONVERT_H__

#include "config.h"

#include <sndfile.h>

namespace MusECore {
class SndFile;
class Xml;

class AudioConverterSettings;
class AudioConverter;
typedef AudioConverter* AudioConverterHandle;
struct AudioConverterDescriptor
{
  // Unique ID of the converter.
  int _ID;
  // Returns combination of Capabilities values.
  int _capabilities;
  // The name of the converter.
  const char* _name;
  // The converter's label (group).
  const char* _label;
  // Maximum available channels. -1 means infinite, don't care.
  int _maxChannels;
  
  // Create an instance of the plugin.
  // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
  AudioConverterHandle (*instantiate)(const struct AudioConverterDescriptor* Descriptor,
                                 int channels, AudioConverterSettings* settings, int mode);
  
  // Destroy the instance after usage.
  void (*cleanup)(AudioConverterHandle Instance);
  
  // Creates a new settings instance. Caller is responsible for deleting the returned object.
  // Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
  AudioConverterSettings* (*createSettings)(bool isLocal);
  
  // Destroy the settings instance after usage.
  void (*cleanupSettings)(AudioConverterSettings* Instance);
};
typedef const AudioConverterDescriptor* 
  (*Audio_Converter_Descriptor_Function)(unsigned long Index);

  
class AudioConverterSettings
{
  public:
    // Three types of settings: 
    // Those suitable for offline work (highest quality vs. slowest is desirable), 
    //  those suitable for realtime work (speed vs quality depends on the end goal), and 
    //  those suitable for gui display (relaxed quality vs fastest is acceptable).
    // They can be OR'd together.
    enum ModeType { OfflineMode = 0x01, RealtimeMode = 0x02, GuiMode = 0x04};
    //static const char* modeNames[];
    
  protected:
    int _converterID;
    
  public:
    AudioConverterSettings(int converterID) : _converterID(converterID) { }
    virtual ~AudioConverterSettings() { }
  
    int converterID() const { return _converterID; }
    
    virtual void assign(const AudioConverterSettings&) = 0;
    
    // Creates another new settings object. Caller is responsible for deleting the returned object.
    // Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
//     virtual AudioConverterSettings* createSettings(bool isLocal) = 0;
  
    virtual int executeUI(int mode, QWidget* parent = NULL, bool isLocal = false) = 0;
    virtual void read(Xml&) = 0;
    virtual void write(int, Xml&) const = 0;
//     // Returns whether any setting is set ie. non-default.
//     // Mode is a combination of AudioConverterSettings::ModeType selecting
//     //  which of the settings to check. Can also be <= 0, meaning all.
//     virtual bool isSet(int mode = -1) const = 0;
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    virtual bool useSettings(int mode = -1) const = 0;
    virtual bool isDefault() const = 0;
};
      
//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

//class AudioConverterSettingsGroup;
class AudioConverter
{
   public:
     // Can be or'd together.
//      enum ConverterID  { SRCResampler=0x01, RubberBand=0x02, ZitaResampler=0x04, SoundTouch=0x08 };
     // Or'd together.
     enum Capabilities { SampleRate=0x01, Stretch=0x02, Pitch=0x04 };
     
   protected:   
      int _channels;
      int _refCount;
      sf_count_t _sfCurFrame;

//       double _sampleRateRatio;
//       double _stretchRatio;
//       double _pitchRatio;
      
   public:   
      AudioConverter();
      virtual ~AudioConverter();
      
      // Returns combination of available ConverterID values.
//       static int availableConverters();
      // Whether the ID is valid and available.
//       static bool isValidID(int converterID);
      
//       // The name of the converter.
//       static const char* name(int converterID);
//       // Returns combination of Capabilities values.
//       static int capabilities(int converterID);
//       // Maximum available channels. -1 means infinite, don't care.
//       static int maxChannels(int converterID);
      
//       static AudioConverterDescriptor* converterDescriptor(int converterID);

      //static AudioConverterSettingsGroup defaultSettings;
      
      // Create an instance of a converter with a number of channels.
//       static AudioConverterHandle create(int converterID, int channels);
      // Reference this instance of a converter. Increases the reference count.
      AudioConverterHandle reference();
      // De-reference this instance of a converter. Decreases the reference count. Destroys the instance if count is zero.
      static AudioConverterHandle release(AudioConverter* cv);
      
//       static void readDefaults(Xml&);
//       static void writeDefaults(int, Xml&);

//       // The name of the converter.
//       virtual const char* name() const = 0;
//       // Returns combination of Capabilities values.
//       virtual int capabilities() const = 0;
//       // Maximum available channels. -1 means infinite, don't care.
//       virtual int maxChannels() const = 0;
      
//       virtual const AudioConverterDescriptor* descriptor() const = 0;

      
//       sf_count_t readAudio(MusECore::SndFileR& sf, unsigned offset, float** buffer, 
//                       int channels, int frames, bool doSeek, bool overwrite);
//       sf_count_t seekAudio(MusECore::SndFileR& sf, sf_count_t offset);
//       sf_count_t readAudio(MusECore::SndFileR sf, unsigned offset, float** buffer, 
//                       int channels, int frames, bool doSeek, bool overwrite);
//       sf_count_t seekAudio(MusECore::SndFileR sf, sf_count_t offset);
//       sf_count_t readAudio(SndFile* sf, unsigned offset, float** buffer, 
//                       int channels, int frames, bool doSeek, bool overwrite);
      
      virtual sf_count_t seekAudio(SndFile* sf, sf_count_t offset);
      
      virtual bool isValid() = 0;
      virtual void reset() = 0;
      virtual void setChannels(int ch) = 0;
//       virtual sf_count_t process(MusECore::SndFileR& sf, float** buffer, 
//                             int channels, int frames, bool overwrite) = 0; // Interleaved buffer if stereo.
//       virtual sf_count_t process(MusECore::SndFileR sf, float** buffer, 
//                             int channels, int frames, bool overwrite) = 0; // Interleaved buffer if stereo.
//       virtual sf_count_t process(SndFile* sf, float** buffer, 
//                             int channels, int frames, bool overwrite) = 0; // Interleaved buffer if stereo.
      virtual int process(SndFile* sf, SNDFILE* handle, sf_count_t pos, float** buffer, 
                            int channels, int frames, bool overwrite) = 0;
                            
      //virtual void read(Xml&) = 0;
      //virtual void write(int, Xml&, const AudioConverterDescriptor* descriptor) const = 0;
      //virtual void write(int, Xml&) const = 0;
                            
};

} // namespace MusECore

#endif

