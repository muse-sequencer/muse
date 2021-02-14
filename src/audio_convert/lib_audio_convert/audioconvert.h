//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.h,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim 
//  (C) Copyright 2009-2020 Tim E. Real (terminator356 A T sourceforge D O T net)
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

//#include "config.h"

#include <QWidget>
#include <sndfile.h>

#include "xml.h"
#include "time_stretch.h"

namespace MusECore {

class AudioConverterSettings
{
  public:
    // Three types of settings: 
    // Those suitable for offline work (highest quality vs. slowest is desirable), 
    //  those suitable for realtime work (speed vs quality depends on the end goal), and 
    //  those suitable for gui display (relaxed quality vs fastest is acceptable).
    // They can be OR'd together.
    enum ModeType { OfflineMode = 0x01, RealtimeMode = 0x02, GuiMode = 0x04};
    
  protected:
    int _converterID;
    
  public:
    AudioConverterSettings(int converterID) : _converterID(converterID) { }
    virtual ~AudioConverterSettings() { }
  
    int converterID() const { return _converterID; }
    
    virtual void assign(const AudioConverterSettings&) = 0;
    
    virtual int executeUI(ModeType mode, QWidget* parent = NULL, bool isLocal = false) = 0;
    virtual void read(Xml&) = 0;
    virtual void write(int, Xml&) const = 0;
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    virtual bool useSettings(int mode = -1) const = 0;
    virtual bool isDefault() const = 0;
};
      
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

  // Minimum and maximum ratios. -1 means infinite, don't care.
  double _minStretchRatio;
  double _maxStretchRatio;
  double _minSamplerateRatio;
  double _maxSamplerateRatio;
  double _minPitchShiftRatio;
  double _maxPitchShiftRatio;

  // Create an instance of the plugin.
  // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
  AudioConverterHandle (*instantiate)(int systemSampleRate, const struct AudioConverterDescriptor* Descriptor,
                                 int channels, AudioConverterSettings* settings, AudioConverterSettings::ModeType mode);
  
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

  
//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

class AudioConverter
{
   public:
     // Can be or'd together.
     enum Capabilities { SampleRate=0x01, Stretch=0x02, Pitch=0x04 };
     
   protected:
      int _systemSampleRate;
      int _channels;
      int _refCount;
      AudioConverterSettings::ModeType _mode;

   public:   
      AudioConverter(int systemSampleRate, AudioConverterSettings::ModeType mode);
      virtual ~AudioConverter();
      
      // Reference this instance of a converter. Increases the reference count.
      AudioConverterHandle reference();
      // De-reference this instance of a converter. Decreases the reference count. Destroys the instance if count is zero.
      static AudioConverterHandle release(AudioConverter* cv);
      
      virtual bool isValid() const = 0;
      virtual void reset() = 0;
      virtual void setChannels(int ch) = 0;

      // Returns the current mode.
      virtual AudioConverterSettings::ModeType mode() const = 0;

      virtual int process(
        SNDFILE* sf_handle,
        const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
        const sf_count_t pos,
        float** buffer, const int channels, const int frames, const bool overwrite) = 0;
};

} // namespace MusECore

#endif

