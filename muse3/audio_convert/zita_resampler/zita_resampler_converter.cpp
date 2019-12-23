//=========================================================
//  MusE
//  Linux Music Editor
//
//  zita_resampler_converter.cpp
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

#include <QRadioButton>
// #include <QListWidgetItem>
// #include <QVariant>
// #include <qtextstream.h>

#include <math.h>
#include <stdio.h>

//#include "zita_resampler_converter.h"
// #include "globals.h"
#include "time_stretch.h"

#include "zita_resampler_converter.h"


// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)



// Create a new instance of the plugin.  
// Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
MusECore::AudioConverter* instantiate(int systemSampleRate,
                                      const MusECore::AudioConverterDescriptor* /*Descriptor*/,
                                      int channels, 
                                      MusECore::AudioConverterSettings* settings, 
                                      int mode)
{
  return new MusECore::ZitaResamplerAudioConverter(systemSampleRate, NULL, channels, settings, mode);  // TODO Pass SF
}

// Destroy the instance after usage.
void cleanup(MusECore::AudioConverterHandle Instance)
{
  MusECore::AudioConverter::release(Instance);
}
  
// Destroy the instance after usage.
void cleanupSettings(MusECore::AudioConverterSettings* instance)
{
  delete instance;
}
  
// Creates a new settings instance. Caller is responsible for deleting the returned object.
// Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
MusECore::AudioConverterSettings* createSettings(bool isLocal)
{
  return new MusECore::ZitaResamplerAudioConverterSettings(isLocal);
}


extern "C" 
{
  static MusECore::AudioConverterDescriptor descriptor = {
    1002,
    MusECore::AudioConverter::SampleRate,
    "Zita Resampler",
    "Zita",
    -1,
    1.0,
    1.0,
    // "The VResampler class provides an arbitrary ratio r in the range 1/16 ≤ r ≤ 64 ..."
    0.0625,
    64.0,
    1.0,
    1.0,
    instantiate,
    cleanup,
    createSettings,
    cleanupSettings
  };

  // We must compile with -fvisibility=hidden to avoid namespace
  // conflicts with global variables.
  // Only visible symbol is "audio_converter_descriptor".
  // (TODO: all plugins should be compiled this way)
  __attribute__ ((visibility("default")))
  
  const MusECore::AudioConverterDescriptor* audio_converter_descriptor(unsigned long i) {
    return (i == 0) ? &descriptor : 0;
  }
}


//==========================================================

namespace MusECore {

//---------------------------------------------------------
//   ZitaResamplerAudioConverter
//---------------------------------------------------------

ZitaResamplerAudioConverter::ZitaResamplerAudioConverter(int systemSampleRate,
                                                         SndFile* sf, 
                                                         int channels, 
                                                         AudioConverterSettings* /*settings*/, 
                                                         int /*mode*/) : AudioConverter(systemSampleRate)
{
  DEBUG_AUDIOCONVERT(stderr, "ZitaResamplerAudioConverter::ZitaResamplerAudioConverter this:%p channels:%d mode:%d\n", 
                     this, channels, mode);

  //_localSettings.initOptions(true);
  
  // "... hlen = 32 should provide very high quality for F_min equal to 
  //  48 kHz or higher, while hlen = 48 should be sufficient for an F_min of 44.1 kHz"
  //
  _filterHLen = 48;
  
// TODO:
//   ZitaResamplerAudioConverterSettings* zita_settings = 
//     static_cast<ZitaResamplerAudioConverterSettings*>(settings);
//
//   switch(mode)
//   {
//     case AudioConverterSettings::OfflineMode:
//       _options = (zita_settings ? zita_settings->offlineOptions()->_options : 0);
//     break;
//     
//     case AudioConverterSettings::RealtimeMode:
//       _options = (zita_settings ? zita_settings->realtimeOptions()->_options : 0);
//     break;
//     
//     case AudioConverterSettings::GuiMode:
//       _options = (zita_settings ? zita_settings->guiOptions()->_options : 0);
//     break;
//   
//     default:
//       _options = 0;
//     break;
//   }
  
  _channels = channels;
  _ratio = sf ? sf->sampleRateRatio() : 1.0;
  
#ifdef ZITA_RESAMPLER_SUPPORT
  _rbs = new VResampler();
#endif
  
#ifdef ZITA_RESAMPLER_SUPPORT
  _rbs->setup(_ratio, _channels, _filterHLen);
#endif
}

ZitaResamplerAudioConverter::~ZitaResamplerAudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "ZitaResamplerAudioConverter::~ZitaResamplerAudioConverter this:%p\n", this);
#ifdef ZITA_RESAMPLER_SUPPORT
  if(_rbs)
    delete _rbs;
#endif
}

void ZitaResamplerAudioConverter::setChannels(int ch)
{
  DEBUG_AUDIOCONVERT(stderr, "ZitaResamplerAudioConverter::setChannels this:%p channels:%d\n", this, ch);
  _channels = ch;
#ifdef ZITA_RESAMPLER_SUPPORT
  if(_rbs)
    delete _rbs;
  _rbs->setup(_ratio, _channels, _filterHLen);
#endif
}

void ZitaResamplerAudioConverter::reset()
{
  DEBUG_AUDIOCONVERT(stderr, "ZitaResamplerAudioConverter::reset this:%p\n", this);
#ifdef ZITA_RESAMPLER_SUPPORT
  if(!_rbs)
    return;
  _rbs->reset();
#endif
  return;  
}

#ifdef ZITA_RESAMPLER_SUPPORT
int ZitaResamplerAudioConverter::process(SndFile* sf, SNDFILE* handle, sf_count_t pos, 
                                         float** buffer, int channels, int frames, bool overwrite)
{
  if(!_rbs || !sf)
    return 0;
  
//   if((MusEGlobal::sampleRate == 0) || (sf->samplerate() == 0))
  if((_systemSampleRate <= 0) || (sf->samplerate() <= 0))
  {  
    DEBUG_AUDIOCONVERT(stderr, "ZitaResamplerAudioConverter::process Error: _systemSampleRate or file samplerate <= 0!\n");
    return 0;
  }  
  
  const double srcratio = sf->sampleRateRatio();
  const double inv_srcratio = 1.0 / srcratio;
  
  const int fchan       = sf->channels();
  
  sf_count_t outFrames  = frames;  
  sf_count_t outSize    = outFrames * fchan;
  
  float* rboutbuffer[fchan];
  float rboutdata[outSize];
  for(int i = 0; i < fchan; ++i)
    rboutbuffer[i] = rboutdata + i * outFrames;
      
  sf_count_t rn           = 0;
  
//   const bool zpad = false; // TODO: Option.
// 
//   int z1, z2;
//   
//   if (zpad)
//   {
//       z1 = _rbs->inpsize () - 1;
//       z2 = _rbs->inpsize () - 1;
//   }
//   else
//   {
//       z1 = _rbs->inpsize () / 2 - 1;
//       z2 = _rbs->inpsize () / 2;
//   }
// 
//   // Insert zero samples at start.
//   _rbs->inp_count = z1;
//   _rbs->inp_data = 0;
//   _rbs->out_count = BUFFSIZE;
//   _rbs->out_data = outb;
// 
//   bool done = false;


// TODO: LEFT OFF HERE....  FINISH THIS....
  _rbs->inp_count = z1;
  _rbs->inp_data = 0;
  _rbs->out_count = BUFFSIZE;
  _rbs->out_data = outb;

  return frames;
}

#else // ZITA_RESAMPLER_SUPPORT

int ZitaResamplerAudioConverter::process(SndFile* /*sf*/, SNDFILE* /*handle*/, sf_count_t /*pos*/, 
                                         float** /*buffer*/, int /*channels*/, int /*frames*/, bool /*overwrite*/)
{
  return 0;
}

#endif // ZITA_RESAMPLER_SUPPORT


//---------------------------------------------------------
//   ZitaResamplerAudioConverterSettings
//---------------------------------------------------------

void ZitaResamplerAudioConverterOptions::write(int level, Xml& xml) const
      {
      //xml.tag(level++, "settings");
      xml.tag(level++, "settings mode=\"%d\"", _mode);
      
      xml.intTag(level, "useSettings", _useSettings);
      //xml.intTag(level, "converterType", _converterType);
      
      xml.tag(--level, "/settings");
      
      }

void ZitaResamplerAudioConverterOptions::read(Xml& xml)
      {
//       int id = -1;
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
                        //else if (tag == "converterType")
                        //      _converterType = xml.parseInt();
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

//---------------------------------------------------------
//   ZitaResamplerAudioConverterSettings
//---------------------------------------------------------

// Some hard-coded defaults.
#ifdef ZITA_RESAMPLER_SUPPORT
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultOfflineOptions(false, AudioConverterSettings::OfflineMode); //, SRC_SINC_BEST_QUALITY);
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultRealtimeOptions(false, AudioConverterSettings::RealtimeMode); //, SRC_SINC_MEDIUM_QUALITY);
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultGuiOptions(false, AudioConverterSettings::GuiMode); //, SRC_SINC_FASTEST);
#else
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultOfflineOptions(false, AudioConverterSettings::OfflineMode); //, 0);
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultRealtimeOptions(false, AudioConverterSettings::RealtimeMode); //, 0);
const ZitaResamplerAudioConverterOptions ZitaResamplerAudioConverterOptions::
      defaultGuiOptions(false, AudioConverterSettings::GuiMode); //, 0);
#endif

ZitaResamplerAudioConverterSettings::ZitaResamplerAudioConverterSettings(
  //int converterID, 
  bool isLocal) 
  //: AudioConverterSettings(converterID)
  : AudioConverterSettings(descriptor._ID)
{ 
  initOptions(isLocal); 
}
      
// MusECore::AudioConverterSettings* ZitaResamplerAudioConverterSettings::createSettings(bool isLocal)
// {
//   return new MusECore::ZitaResamplerAudioConverterSettings(isLocal);
// }

void ZitaResamplerAudioConverterSettings::assign(const AudioConverterSettings& other)
{
  const ZitaResamplerAudioConverterSettings& zita_other = 
    (const ZitaResamplerAudioConverterSettings&)other;
  _offlineOptions  = zita_other._offlineOptions;
  _realtimeOptions = zita_other._realtimeOptions;
  _guiOptions      = zita_other._guiOptions;
}

// bool ZitaResamplerAudioConverterSettings::isSet(int mode) const 
// { 
//   if(mode & ~(AudioConverterSettings::OfflineMode | 
//               AudioConverterSettings::RealtimeMode | 
//               AudioConverterSettings::GuiMode))
//     fprintf(stderr, "ZitaResamplerAudioConverterSettings::isSet() Warning: Unknown modes included:%d\n", mode);
//   
//   if((mode <= 0 || (mode & AudioConverterSettings::OfflineMode)) && _offlineOptions.isSet())
//     return true;
// 
//   if((mode <= 0 || (mode & AudioConverterSettings::RealtimeMode)) && _realtimeOptions.isSet())
//     return true;
// 
//   if((mode <= 0 || (mode & AudioConverterSettings::GuiMode)) && _guiOptions.isSet())
//     return true;
//     
//   return false;
// }

bool ZitaResamplerAudioConverterSettings::useSettings(int mode) const 
{ 
  if(mode > 0 &&
     (mode & ~(AudioConverterSettings::OfflineMode |
              AudioConverterSettings::RealtimeMode | 
              AudioConverterSettings::GuiMode)))
    fprintf(stderr, "ZitaResamplerAudioConverterSettings::useSettings() Warning: Unknown modes included:%d\n", mode);
  
  if((mode <= 0 || (mode & AudioConverterSettings::OfflineMode)) && _offlineOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::RealtimeMode)) && _realtimeOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::GuiMode)) && _guiOptions.useSettings())
    return true;
    
  return false;
}

int ZitaResamplerAudioConverterSettings::executeUI(int mode, QWidget* parent, bool isLocal) 
{
  MusEGui::ZitaResamplerSettingsDialog dlg(mode, parent, this, isLocal);
  return dlg.exec(); 
}

void ZitaResamplerAudioConverterSettings::write(int level, Xml& xml) const
{
//   const bool use_off = !(_offlineOptions == defaultOfflineOptions);
//   const bool use_rt  = !(_realtimeOptions == defaultRealtimeOptions);
//   const bool use_gui = !(_guiOptions == defaultGuiOptions);
// 
//   if(use_off | use_rt || use_gui)
//   {
//     xml.tag(level++, "zitaResamplerSettings");
//     
//     if(use_off)
//     {
//       xml.tag(level++, "offline");
//       _offlineOptions.write(level, xml);
//       xml.tag(--level, "/offline");
//     }
//     
//     if(use_rt)
//     {
//       xml.tag(level++, "realtime");
//       _realtimeOptions.write(level, xml);
//       xml.tag(--level, "/realtime");
//     }
//     
//     if(use_gui)
//     {
//       xml.tag(level++, "gui");
//       _guiOptions.write(level, xml);
//       xml.tag(--level, "/gui");
//     }
//     
//     xml.tag(--level, "/zitaResamplerSettings");
//   }
  
  const bool use_off = !(_offlineOptions == ZitaResamplerAudioConverterOptions::defaultOfflineOptions);
  const bool use_rt  = !(_realtimeOptions == ZitaResamplerAudioConverterOptions::defaultRealtimeOptions);
  const bool use_gui = !(_guiOptions == ZitaResamplerAudioConverterOptions::defaultGuiOptions);

  if(use_off | use_rt || use_gui)
  {
    //xml.tag(level++, "audioConverterSetting id=\"%d\"", descriptor._ID);
    //xml.tag(level++, "audioConverterSetting name=\"%s\"", descriptor._name);
    xml.tag(level++, "audioConverterSetting name=\"%s\"", Xml::xmlString(descriptor._name).toLatin1().constData());
    //xml.tag(level++, descriptor._name);
    
    if(use_off)
    {
      //xml.tag(level++, "offline");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::OfflineMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::OfflineMode);
      _offlineOptions.write(level, xml);
      //xml.tag(--level, "/offline");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    if(use_rt)
    {
      //xml.tag(level++, "realtime");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::RealtimeMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::RealtimeMode);
      _realtimeOptions.write(level, xml);
      //xml.tag(--level, "/realtime");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    if(use_gui)
    {
      //xml.tag(level++, "gui");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::GuiMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::GuiMode);
      _guiOptions.write(level, xml);
      //xml.tag(--level, "/gui");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    xml.tag(--level, "/audioConverterSetting");
  }
}

void ZitaResamplerAudioConverterSettings::read(Xml& xml)
      {
      int mode = -1;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
//                         if (tag == "offline")
//                               _offlineOptions.read(xml);
//                         else if (tag == "realtime")
//                               _realtimeOptions.read(xml);
//                         else if (tag == "gui")
//                               _guiOptions.read(xml);
                        
                        if(mode != -1)
                        {
                          ZitaResamplerAudioConverterOptions* opts = NULL;
                          switch(mode)
                          {
                            case AudioConverterSettings::OfflineMode:
                              opts = &_offlineOptions;
                            break;

                            case AudioConverterSettings::RealtimeMode:
                              opts = &_realtimeOptions;
                            break;
                            
                            case AudioConverterSettings::GuiMode:
                              opts = &_guiOptions;
                            break;
                          }
                          
                          if(opts)
                          {
                            if(tag == "useSettings")
                              opts->_useSettings = xml.parseInt();
                            //else 
                            //  if(tag == "converterType")
                            //  opts->_converterType = xml.parseInt();
                          }
                        }
                  
                        else
                              //xml.unknown("zitaResamplerSettings");
                              xml.unknown("settings");
                        break;
                  case Xml::Attribut:
                        if (tag == "mode")
                            mode = xml.s2().toInt();
                        else
                              //fprintf(stderr, "zitaResamplerSettings unknown tag %s\n", tag.toLatin1().constData());
                              fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        //if (tag == "zitaResamplerSettings") {
                        if (tag == "settings") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }
      
} // namespace MusECore


//==========================================================


namespace MusEGui {
  
ZitaResamplerSettingsDialog::ZitaResamplerSettingsDialog(
  int mode, 
  QWidget* parent, 
  MusECore::AudioConverterSettings* settings, 
  bool isLocal)
  : QDialog(parent)
{
  setupUi(this);
  
  OKButton->setEnabled(false);
  
  _options = NULL;
  if(settings)
  {
    MusECore::ZitaResamplerAudioConverterSettings* zita_settings = 
      static_cast<MusECore::ZitaResamplerAudioConverterSettings*>(settings);
    
    switch(mode)
    {
      case MusECore::AudioConverterSettings::OfflineMode:
        _options = zita_settings->offlineOptions();
      break;

      case MusECore::AudioConverterSettings::RealtimeMode:
        _options = zita_settings->realtimeOptions();
      break;

      case MusECore::AudioConverterSettings::GuiMode:
        _options = zita_settings->guiOptions();
      break;
      
      default:
        // Disable everything and return.
      break;
    }
  }

#ifdef ZITA_RESAMPLER_SUPPORT
  //if(isLocal)
    useDefaultSettings->setChecked(!_options || !_options->_useSettings);
  useDefaultSettings->setEnabled(isLocal && _options);
  useDefaultSettings->setVisible(isLocal && _options);
  optionsGroup->setEnabled(!isLocal || (_options && _options->_useSettings));
  warningLabel->setVisible(false);
#else
  useDefaultSettings->setEnabled(false);
  useDefaultSettings->setVisible(isLocal && _options);
  optionsGroup->setEnabled(false);
  warningLabel->setVisible(true);
#endif
  
  setControls();

//   connect(typeSINCBestQuality, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
//   connect(typeSINCMedium, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
//   connect(typeSINCFastest, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
//   connect(typeZeroOrderHold, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
//   connect(typeLinear, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(useDefaultSettings, &QCheckBox::clicked, [this]() { buttonClicked(DefaultsButtonId); } );
  connect(OKButton, &QPushButton::clicked, [this]() { buttonClicked(OkButtonId); } );
  connect(cancelButton, &QPushButton::clicked, [this]() { buttonClicked(CancelButtonId); } );
}
  
void ZitaResamplerSettingsDialog::setControls()
{
#ifdef ZITA_RESAMPLER_SUPPORT
  
  if(!_options)
    return;
  
//   switch(_options->_converterType)
//   {
//     case SRC_SINC_BEST_QUALITY:
//       typeSINCBestQuality->blockSignals(true);
//       typeSINCBestQuality->setChecked(true);
//       typeSINCBestQuality->blockSignals(false);
//     break;
//     
//     case SRC_SINC_MEDIUM_QUALITY:
//       typeSINCMedium->blockSignals(true);
//       typeSINCMedium->setChecked(true);
//       typeSINCMedium->blockSignals(false);
//     break;
//     
//     case SRC_SINC_FASTEST:
//       typeSINCFastest->blockSignals(true);
//       typeSINCFastest->setChecked(true);
//       typeSINCFastest->blockSignals(false);
//     break;
//     
//     case SRC_ZERO_ORDER_HOLD:
//       typeZeroOrderHold->blockSignals(true);
//       typeZeroOrderHold->setChecked(true);
//       typeZeroOrderHold->blockSignals(false);
//     break;
//     
//     case SRC_LINEAR:
//       typeLinear->blockSignals(true);
//       typeLinear->setChecked(true);
//       typeLinear->blockSignals(false);
//     break;
/*   
    default:
    break;
  }*/

  //optionsGroup->setEnabled(!useDefaultSettings->isVisible() || _options->_useSettings);
  
#endif
}

void ZitaResamplerSettingsDialog::buttonClicked(int idx)
{
  switch(idx)
  {
    case DefaultsButtonId:
      OKButton->setEnabled(true);
      //setControls();
      //optionsGroup->setEnabled(_options->_useSettings);
      optionsGroup->setEnabled(!useDefaultSettings->isChecked());
    break;
    
    case ConverterButtonId:
      OKButton->setEnabled(true);
    break;
    
    case OkButtonId:
      accept();
    break;
    
    case CancelButtonId:
      reject();
    break;
    
    default:
    break;
  }
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void ZitaResamplerSettingsDialog::accept()
{
  if(!_options)
  {
    QDialog::accept();
    return;
  }
  
#ifdef ZITA_RESAMPLER_SUPPORT
  
//   int type = -1;
//   if(typeSINCBestQuality->isChecked())
//     type = SRC_SINC_BEST_QUALITY;
//   else if(typeSINCMedium->isChecked())
//     type = SRC_SINC_MEDIUM_QUALITY;
//   else if(typeSINCFastest->isChecked())
//     type = SRC_SINC_FASTEST;
//   else if(typeZeroOrderHold->isChecked())
//     type = SRC_ZERO_ORDER_HOLD;
//   else if(typeLinear->isChecked())
//     type = SRC_LINEAR;
//   
//   if(type != -1)
//     _options->_converterType = type;

  _options->_useSettings = !useDefaultSettings->isChecked();
  
#endif

  QDialog::accept();
}

} // namespace MusEGui


