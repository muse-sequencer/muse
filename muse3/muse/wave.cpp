//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wave.cpp,v 1.19.2.20 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cmath>
#include <samplerate.h>

#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>

//#include "xml.h"
#include "song.h"
#include "wave.h"
#include "app.h"
#include "filedialog.h"
#include "arranger/arranger.h"
#include "globals.h"
#include "event.h"
#include "audio.h"
#include "sig.h"
#include "part.h"
#include "track.h"
#include "wavepreview.h"
#include "gconfig.h"
#include "type_defs.h"

// REMOVE Tim. samplerate. Added.
#include "config.h"
// #include "operations.h"
#include "time_stretch.h"
//#include "audioconvert.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"

//#define WAVE_DEBUG
//#define WAVE_DEBUG_PRC

// REMOVE Tim. samplerate. Added.
#define USE_SAMPLERATE
#define ERROR_WAVE(dev, format, args...)  fprintf(dev, format, ##args)
#define INFO_WAVE(dev, format, args...)  fprintf(dev, format, ##args)
// For debugging output: Uncomment the fprintf section.
#define DEBUG_WAVE(dev, format, args...)  // fprintf(dev, format, ##args)

namespace MusECore {

const int cacheMag = 128;


SndFileList SndFile::sndFiles;

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

SndFile::SndFile(const QString& name)
      {
      // REMOVE Tim. samplerate. Added.
      _stretchList = new StretchList();
      //_audioConverterSettings = new AudioConverterSettingsGroup(true);  // true = Local settings, initialized to -1.
      
      // true = Local settings, initialized to -1.
      _audioConverterSettings = new AudioConverterSettingsGroup(true); // Local settings.
        //MusEGlobal::defaultAudioConverterSettings.createSettings(&MusEGlobal::audioConverterPluginList, true);
        //new AudioConverterSettingsGroup(&MusEGlobal::audioConverterPluginList, true);
      _audioConverterSettings->populate(&MusEGlobal::audioConverterPluginList, true);
      
      finfo = new QFileInfo(name);
      sf    = NULL;
      sfUI  = NULL;
      csize = 0;
      cache = NULL;
      openFlag = false;
      sndFiles.push_back(this);
      refCount = 0;
      writeBuffer = NULL;
      writeSegSize = std::max((size_t)MusEGlobal::segmentSize, (size_t)cacheMag);// cache minimum segment size for write operations
      
      // REMOVE Tim. samplerate. Added.
      _staticAudioConverter    = NULL;
      _staticAudioConverterUI  = NULL;
      _dynamicAudioConverter   = NULL;
      _dynamicAudioConverterUI = NULL;
      }

SndFile::~SndFile()
      {
      //DEBUG_WAVE(stderr, "SndFile:: already open\n");
      INFO_WAVE(stderr, "SndFile dtor this:%p\n", this); // REMOVE Tim. samplerate. Added. TESTING Change back to DEBUG_WAVE!
      if (openFlag)
            close();
      for (iSndFile i = sndFiles.begin(); i != sndFiles.end(); ++i) {
            if (*i == this) {
                  //DEBUG_WAVE(stderr, "erasing from sndfiles:%s\n", finfo->canonicalFilePath().toLatin1().constData());
                  sndFiles.erase(i);
                  break;
                  }
            }
      delete finfo;
      if (cache) {
            delete[] cache;
            cache = NULL;
            }
      if(writeBuffer) {
         delete [] writeBuffer;
          writeBuffer = NULL;
      }

      // REMOVE Tim. samplerate. Added.
      delete _stretchList;
      delete _audioConverterSettings;
      }

//---------------------------------------------------------
//   openRead
//---------------------------------------------------------

bool SndFile::openRead(bool createCache, bool showProgress)
      {
      if (openFlag) {
            DEBUG_WAVE(stderr, "SndFile:: already open\n");
            return false;
            }
      QString p = path();
      sfinfo.format = 0;
      sfUI = NULL;
      sf = sf_open(p.toLocal8Bit().constData(), SFM_READ, &sfinfo);
      if (!sf)
            return true;
      if(createCache){
         sfinfo.format = 0;
         sfUI = sf_open(p.toLocal8Bit().constData(), SFM_READ, &sfinfo);
         if (!sfUI){
            sf_close(sf);
            sf = NULL;
            return true;
         }
      }

      // REMOVE Tim. samplerate. Added.
      const StretchList* sl = stretchList();
      _staticAudioConverter   = setupAudioConverter(audioConverterSettings(), 
                                                    true,  // true = Local settings.
                                                    AudioConverterSettings::RealtimeMode,
                                                    sl->isResampled(),
                                                    sl->isStretched());
      _staticAudioConverterUI = setupAudioConverter(audioConverterSettings(), 
                                                    true,  // true = Local settings.
                                                    AudioConverterSettings::GuiMode, 
                                                    sl->isResampled(),
                                                    sl->isStretched());
      
      writeFlag = false;
      openFlag  = true;
      if (createCache) {
        QString cacheName = finfo->absolutePath() + QString("/") + finfo->completeBaseName() + QString(".wca");
        
// REMOVE Tim. samplerate. Added.
// #ifdef USE_SAMPLERATE
//         readCacheConverted(cacheName, showProgress);
// #else
        
        readCache(cacheName, showProgress);
// #endif
        
      }
      return false;
      }

// REMOVE Tim. samplerate. Added.
//bool SndFile::setupAudioConverter(AudioConverterSettingsGroup* settings, AudioConverterPluginI** p_plugI, int mode)
AudioConverterPluginI* SndFile::setupAudioConverter(const AudioConverterSettingsGroup* settings, 
                                                    bool isLocalSettings, 
                                                    int mode, 
                                                    bool doResample,
                                                    bool doStretch) const
{
#ifdef USE_SAMPLERATE

  if(!MusEGlobal::defaultAudioConverterSettings)
    return NULL;
  
//   if(!strList)
//     strList = stretchList();
  
  AudioConverterPluginI* plugI = NULL;
  
  int pref_resampler = 
    //(settings && settings->_options._useSettings) ? 
    (settings && (settings->_options._useSettings || !isLocalSettings)) ? 
      settings->_options._preferredResampler :
      //MusEGlobal::defaultAudioConverterSettings._options._preferredResampler;
      MusEGlobal::defaultAudioConverterSettings->_options._preferredResampler;
  
  int pref_shifter = 
    //(settings && settings->_options._useSettings) ? 
    (settings && (settings->_options._useSettings || !isLocalSettings)) ? 
      settings->_options._preferredShifter:
      //MusEGlobal::defaultAudioConverterSettings._options._preferredShifter;
      MusEGlobal::defaultAudioConverterSettings->_options._preferredShifter;

  AudioConverterSettingsI* def_res_settings = NULL;
  AudioConverterSettingsI* local_res_settings = NULL;
  AudioConverterSettingsI* rt_res_settings = NULL;
//   AudioConverterSettingsI* gui_res_settings = NULL;
  AudioConverterPlugin* res_plugin = MusEGlobal::audioConverterPluginList.find(0, pref_resampler, AudioConverter::SampleRate);
  if(res_plugin)
  {
    if(isLocalSettings)
    {
      //def_res_settings = MusEGlobal::defaultAudioConverterSettings.find(
      //    MusEGlobal::defaultAudioConverterSettings._options._preferredResampler);
      def_res_settings = MusEGlobal::defaultAudioConverterSettings->find(
          MusEGlobal::defaultAudioConverterSettings->_options._preferredResampler);
      local_res_settings = settings ? settings->find(pref_resampler) : NULL;
  //     rt_res_settings = (local_res_settings && local_res_settings->useSettings(AudioConverterSettings::RealtimeMode)) ? 
      rt_res_settings = (local_res_settings && local_res_settings->useSettings(mode)) ? local_res_settings : def_res_settings;
  //     gui_res_settings = (local_res_settings && local_res_settings->useSettings(AudioConverterSettings::GuiMode)) ? 
  //       local_res_settings : def_res_settings;
    }
    else
      rt_res_settings = settings->find(pref_resampler);
  }

  AudioConverterSettingsI* def_str_settings = NULL;
  AudioConverterSettingsI* local_str_settings = NULL;
  AudioConverterSettingsI* rt_str_settings = NULL;
//   AudioConverterSettingsI* gui_str_settings = NULL;
  AudioConverterPlugin* str_plugin = MusEGlobal::audioConverterPluginList.find(0, pref_shifter, AudioConverter::Stretch);
  if(str_plugin)
  {
    if(isLocalSettings)
    {
      //def_str_settings = MusEGlobal::defaultAudioConverterSettings.find(
      //    MusEGlobal::defaultAudioConverterSettings._options._preferredShifter);
      def_str_settings = MusEGlobal::defaultAudioConverterSettings->find(
          MusEGlobal::defaultAudioConverterSettings->_options._preferredShifter);
      local_str_settings = settings ? settings->find(pref_shifter) : NULL;
      //rt_str_settings = (local_str_settings && local_str_settings->useSettings(AudioConverterSettings::RealtimeMode)) ? 
      rt_str_settings = (local_str_settings && local_str_settings->useSettings(mode)) ? local_str_settings : def_str_settings;
      
      if(rt_str_settings) { } // Dummy for now, to avoid unused warning.
      
  //     gui_str_settings = (local_str_settings && local_str_settings->useSettings(AudioConverterSettings::GuiMode)) ? 
  //       local_str_settings : def_str_settings;
    }
    else
      rt_str_settings = settings->find(pref_shifter);
  }

  
  //if(sf)
//   if(sf && (sampleRateDiffers() || strList->isStretched()))
  //if(sf && (sampleRateDiffers() || strList->isResampled() || strList->isStretched()))
  if(sf && (sampleRateDiffers() || doResample || doStretch))
  {
    // Do we already have a valid audio converter?
    //if(_staticAudioConverter)
//     if(plugI)
      // Just set the channels.
      //_staticAudioConverter->setChannels(sfinfo.channels);
//       plugI->setChannels(sfinfo.channels);
//     else
    {  
      // Realtime audio section...
      // Create a new converter.
      
      AudioConverterPlugin* fin_plug  = 
        doStretch ? str_plugin : (res_plugin ? res_plugin : str_plugin);
        
      AudioConverterSettingsI* fin_set = 
        doStretch ? rt_str_settings : (rt_res_settings ? rt_res_settings : rt_str_settings);
        
      if(fin_set && fin_plug && (fin_plug->maxChannels() < 0 || sfinfo.channels <= fin_plug->maxChannels()))
      {
        //_staticAudioConverter = new AudioConverterPluginI();
        plugI = new AudioConverterPluginI();
        //_staticAudioConverter->initPluginInstance(
        plugI->initPluginInstance(
          fin_plug,
          sfinfo.channels,
          fin_set->settings(), 
          //AudioConverterSettings::RealtimeMode);
          mode);
      }
    } 
  }
//   //else if(_staticAudioConverter)
//   else if(plugI)
//   {
//     //_staticAudioConverter = AudioConverter::release(_staticAudioConverter);
//     //delete _staticAudioConverter;
//     delete plugI;
//     //_staticAudioConverter = NULL;
//     plugI = NULL;
//   }
    
//   //if(sfUI)
//   if(sfUI && (sampleRateDiffers() || stretchList()->isStretched()))
//   {
//     // Do we already have a valid audio converter?
//     if(_staticAudioConverterUI)
//       // Just set the channels.
//       _staticAudioConverterUI->setChannels(sfinfo.channels);
//     else
//     {
//       // Gui section...
//       // Create a new converter.
//       if(gui_res_settings)
//       {
//         _staticAudioConverterUI = new AudioConverterPluginI();
//         _staticAudioConverterUI->initPluginInstance(
//           res_plugin,
//           sfinfo.channels,
//           gui_res_settings->settings(),
//           AudioConverterSettings::GuiMode);
//       }
//     }
//   }
//   else if(_staticAudioConverterUI)
//   {
//     //_staticAudioConverterUI = AudioConverter::release(_staticAudioConverterUI);
//     delete _staticAudioConverterUI;
//     _staticAudioConverterUI = NULL;
//   }
  
return plugI;

#else

return NULL;

#endif
}

AudioConverterPluginI* SndFile::staticAudioConverter(int mode) const 
{ 
  switch(mode)
  {
    case AudioConverterSettings::OfflineMode:
      return NULL;  // TODO ?
    break;

    case AudioConverterSettings::RealtimeMode:
      return _staticAudioConverter;
    break;

    case AudioConverterSettings::GuiMode:
      return _staticAudioConverterUI;
    break;
  }
  return NULL; 
}

void SndFile::setStaticAudioConverter(AudioConverterPluginI* converter, int mode)
{
  switch(mode)
  {
    case AudioConverterSettings::OfflineMode:
      return;  // TODO ?
    break;

    case AudioConverterSettings::RealtimeMode:
      _staticAudioConverter = converter;
    break;

    case AudioConverterSettings::GuiMode:
      _staticAudioConverterUI = converter;
    break;
  }
}

double SndFile::minStretchRatio() const
{
  double m = 0.0;
  if(_staticAudioConverter && _staticAudioConverter->minStretchRatio() > m)
    m = _staticAudioConverter->minStretchRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->minStretchRatio() > m)
    m = _staticAudioConverterUI->minStretchRatio();
  return m;
}

double SndFile::maxStretchRatio() const
{
  double m = -1.0;
  if(_staticAudioConverter && _staticAudioConverter->maxStretchRatio() > 0.0 && (m < 0.0 || _staticAudioConverter->maxStretchRatio() < m))
    m = _staticAudioConverter->maxStretchRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxStretchRatio() > 0.0 && (m < 0.0 || _staticAudioConverterUI->maxStretchRatio() < m))
    m = _staticAudioConverterUI->maxStretchRatio();
  return m;
}

double SndFile::minSamplerateRatio() const
{
  double m = 0.0;
  if(_staticAudioConverter && _staticAudioConverter->minSamplerateRatio() > m)
    m = _staticAudioConverter->minSamplerateRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->minSamplerateRatio() > m)
    m = _staticAudioConverterUI->minSamplerateRatio();
  return m;
}

double SndFile::maxSamplerateRatio() const
{
  double m = -1.0;
  if(_staticAudioConverter && _staticAudioConverter->maxSamplerateRatio() > 0.0 && (m < 0.0 || _staticAudioConverter->maxSamplerateRatio() < m))
    m = _staticAudioConverter->maxSamplerateRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxSamplerateRatio() > 0.0 && (m < 0.0 || _staticAudioConverterUI->maxSamplerateRatio() < m))
    m = _staticAudioConverterUI->maxStretchRatio();
  return m;
}

double SndFile::minPitchShiftRatio() const
{
  double m = 0.0;
  if(_staticAudioConverter && _staticAudioConverter->minPitchShiftRatio() > m)
    m = _staticAudioConverter->minPitchShiftRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->minPitchShiftRatio() > m)
    m = _staticAudioConverterUI->minPitchShiftRatio();
  return m;
}

double SndFile::maxPitchShiftRatio() const
{
  double m = -1.0;
  if(_staticAudioConverter && _staticAudioConverter->maxPitchShiftRatio() > 0.0 && (m < 0.0 || _staticAudioConverter->maxPitchShiftRatio() < m))
    m = _staticAudioConverter->maxPitchShiftRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxPitchShiftRatio() > 0.0 && (m < 0.0 || _staticAudioConverterUI->maxPitchShiftRatio() < m))
    m = _staticAudioConverterUI->maxPitchShiftRatio();
  return m;
}

void SndFile::setAudioConverterSettings(AudioConverterSettingsGroup* settings)
{ 
  _audioConverterSettings = settings;
}

// void SndFile::modifyAudioConverterSettingsOperation(
//   AudioConverterSettingsGroup* settings,
//   bool isLocalSettings,
//   PendingOperationList& ops) //,
//   //bool doResample,
//   //bool doStretch)
// {
//   const StretchList* sl = stretchList();
//   const bool doStretch = sl->isStretched();
//   const bool doResample = sl->isResampled();
//   AudioConverterPluginI* converter   = setupAudioConverter(settings,
//                                                            isLocalSettings,
//                                                            AudioConverterSettings::RealtimeMode,
//                                                            doResample,
//                                                            doStretch);
//   AudioConverterPluginI* converterUI = setupAudioConverter(settings,
//                                                            isLocalSettings,
//                                                            AudioConverterSettings::GuiMode,
//                                                            doResample,
//                                                            doStretch);
// 
// //   if(!converter && !converterUI)
// //     return;
// 
//   // We want to change the settings, and the converters if neccesary.
//   ops.add(PendingOperationItem(this, settings, converter, converterUI, PendingOperationItem::ModifyLocalAudioConverterSettings));
// }
// 
// void SndFile::modifyAudioConverterOperation(
//   //AudioConverterSettingsGroup* settings, 
//   //bool isLocalSettings, 
//   PendingOperationList& ops,
//   bool doResample,
//   bool doStretch)
// {
//   AudioConverterSettingsGroup* settings = _audioConverterSettings->useSettings() ?
//     _audioConverterSettings : MusEGlobal::defaultAudioConverterSettings;
// 
//   const bool isLocalSettings = _audioConverterSettings->useSettings();
// 
//   AudioConverterPluginI* converter   = setupAudioConverter(settings,
//                                                            isLocalSettings,
//                                                            AudioConverterSettings::RealtimeMode,
//                                                            doResample,
//                                                            doStretch);
//   AudioConverterPluginI* converterUI = setupAudioConverter(settings,
//                                                            isLocalSettings,
//                                                            AudioConverterSettings::GuiMode,
//                                                            doResample,
//                                                            doStretch);
// 
// //   if(!converter && !converterUI)
// //     return;
// 
//   // NULL = we only want to change the converters, not the settings.
//   ops.add(PendingOperationItem(this, NULL, converter, converterUI, PendingOperationItem::ModifyLocalAudioConverterSettings));
// }
// 
// void SndFile::modifyStretchListOperation(int type, double value, PendingOperationList& ops)
// {
//   ops.add(PendingOperationItem(type, stretchList(), value, PendingOperationItem::ModifyStretchListRatio));
// }
// 
// void SndFile::addAtStretchListOperation(int type, MuseFrame_t frame, double value, PendingOperationList& ops)
// {
//   StretchList* sl = stretchList();
// //   iStretchListItem ie = sl->find(frame);
// //   if(ie != sl->end())
// //     ops.add(PendingOperationItem(type, sl, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
// //   else
// //     ops.add(PendingOperationItem(type, sl, frame, value, PendingOperationItem::AddStretchListRatioAt));
//   stretchList()->addListOperation(StretchListItem::StretchEventType(type), frame, value, ops);
//   
//   bool wantStretch = false;
//   bool wantResample = sampleRateDiffers();
//   bool wantPitch = false;
//   const bool haveStretch = sl->isStretched();
//   const bool haveResample = sl->isResampled() || sampleRateDiffers();
//   const bool havePitch = sl->isPitchShifted();
//   
//   //// If the requested value is anything other than 1.0, request converters.
//   //if(value != 1.0)
//   //{
//     switch(type)
//     {
//       case StretchListItem::StretchEvent:
//         wantStretch = true;
//       break;
//       case StretchListItem::SamplerateEvent:
//         wantResample = true;
//       break;
//       case StretchListItem::PitchEvent:
//         wantPitch = true;
//       break;
//     }
//   //}
//   
//   if((wantStretch  && !haveStretch) || 
//      (wantResample && !haveResample) || 
//      (wantPitch    && !havePitch))
//   {
//     const bool doStretch  = wantStretch  ? true : haveStretch;
//     const bool doResample = wantResample ? true : haveResample;
//   //const bool doPitch    = wantPitch    ? true : havePitch;
//     
//     modifyAudioConverterOperation(ops, doResample, doStretch);
//   }
// }
// 
// void SndFile::delAtStretchListOperation(int types, MuseFrame_t frame, PendingOperationList& ops)
// {
//   // Do not delete the item at zeroth frame.
//   if(frame == 0)
//     return;
//   
//   StretchList* sl = stretchList();
// //   iStretchListItem e = sl->find(frame);
// //   if (e == sl->end()) {
// //         ERROR_WAVE(stderr, "SndFile::delAtStretchListOperation frame:%ld not found\n", frame);
// //         return;
// //         }
// //   PendingOperationItem poi(types, sl, e, PendingOperationItem::DeleteStretchListRatioAt);
// //   // NOTE: Deletion is done in post-RT stage 3.
// //   ops.add(poi);
//   sl->delListOperation(types, frame, ops);
// 
//   StretchListInfo info =  sl->testDelListOperation(types, frame);
//   
//   const bool wantStretch  = info._isStretched;
//   const bool wantResample = info._isResampled || sampleRateDiffers();
//   const bool wantPitch    = info._isPitchShifted;
//   
//   const bool haveStretch  = sl->isStretched();
//   const bool haveResample = sl->isResampled() || sampleRateDiffers();
//   const bool havePitch    = sl->isPitchShifted();
//   
//   if((!wantStretch  && haveStretch) || 
//      (!wantResample && haveResample) || 
//      (!wantPitch    && havePitch))
//   {
//     const bool doStretch  = !wantStretch  ? false : haveStretch;
//     const bool doResample = !wantResample ? false : haveResample;
//   //const bool doPitch    = !wantPitch    ? false : havePitch;
//     
//     modifyAudioConverterOperation(ops, doResample, doStretch);
//   }
// }
// 
// void SndFile::modifyAtStretchListOperation(int type, MuseFrame_t frame, double value, PendingOperationList& ops)
// {
//   StretchList* sl = stretchList();
// //   iStretchListItem ie = sl->find(frame);
// //   if(ie == sl->end()) {
// //         ERROR_WAVE(stderr, "SndFile::modifyAtStretchListOperation frame:%ld not found\n", frame);
// //         return;
// //         }
// //   ops.add(PendingOperationItem(type, sl, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
//   sl->modifyListOperation(StretchListItem::StretchEventType(type), frame, value, ops);
//   
//   bool wantStretch = false;
//   bool wantResample = sampleRateDiffers();
//   bool wantPitch = false;
//   const bool haveStretch = sl->isStretched();
//   const bool haveResample = sl->isResampled() || sampleRateDiffers();
//   const bool havePitch = sl->isPitchShifted();
//   
//   //// If the requested value is anything other than 1.0, request converters.
//   //if(value != 1.0)
//   //{
//     switch(type)
//     {
//       case StretchListItem::StretchEvent:
//         wantStretch = true;
//       break;
//       case StretchListItem::SamplerateEvent:
//         wantResample = true;
//       break;
//       case StretchListItem::PitchEvent:
//         wantPitch = true;
//       break;
//     }
//   //}
//   
//   if((wantStretch  && !haveStretch) || 
//      (wantResample && !haveResample) || 
//      (wantPitch    && !havePitch))
//   {
//     const bool doStretch  = wantStretch  ? true : haveStretch;
//     const bool doResample = wantResample ? true : haveResample;
//   //const bool doPitch    = wantPitch    ? true : havePitch;
//     
//     modifyAudioConverterOperation(ops, doResample, doStretch);
//   }
// }

//---------------------------------------------------------
//   update
//    called after recording to file
//---------------------------------------------------------

void SndFile::update(bool showProgress)
      {
      close();

      // force recreation of wca data
      QString cacheName = finfo->absolutePath() +
         QString("/") + finfo->completeBaseName() + QString(".wca");
      ::remove(cacheName.toLocal8Bit().constData());
      if (openRead(true, showProgress)) {
            ERROR_WAVE(stderr, "SndFile::update openRead(%s) failed: %s\n", path().toLocal8Bit().constData(), strerror().toLocal8Bit().constData());
            }
      }

//---------------------------------------------------
//  create cache
//---------------------------------------------------

void SndFile::createCache(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart)
{
   if(cstart >= csize)
      return;
   QProgressDialog* progress = NULL;
   if (showProgress) {
      QString label(QWidget::tr("create peakfile for "));
      label += basename();
      progress = new QProgressDialog(label,
                                     QString::null, 0, csize, 0);
      progress->setMinimumDuration(0);
      progress->show();
   }
   
   const int srcChannels = channels();
   float data[srcChannels][cacheMag];
   float* fp[srcChannels];
   for (int k = 0; k < srcChannels; ++k)
      fp[k] = &data[k][0];
   int interval = (csize - cstart) / 10;

   if(!interval)
      interval = 1;
   for (int i = cstart; i < csize; i++) {
      if (showProgress && ((i % interval) == 0))
         progress->setValue(i);
      seek(i * cacheMag, 0);
      read(srcChannels, fp, cacheMag);
      for (int ch = 0; ch < srcChannels; ++ch) {
         float rms = 0.0;
         cache[ch][i].peak = 0;
         for (int n = 0; n < cacheMag; n++) {
            float fd = data[ch][n];
            rms += fd * fd;
            int idata = int(fd * 255.0);
            if (idata < 0)
               idata = -idata;
            if (cache[ch][i].peak < idata)
               cache[ch][i].peak = idata;
         }
         // amplify rms value +12dB
         int rmsValue = int((sqrt(rms/cacheMag) * 255.0));
         if (rmsValue > 255)
            rmsValue = 255;
         cache[ch][i].rms = rmsValue;
      }
   }
   if (showProgress)
      progress->setValue(csize);
   if(bWrite)
      writeCache(path);
   if (showProgress)
      delete progress;

}

//---------------------------------------------------
//  createCacheConverted
//---------------------------------------------------

// void SndFile::createCacheConverted(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart)
// {
//     if(!sampleRateDiffers() || !_audConvUI)
//     {
//       createCache(path, showProgress, bWrite, cstart);
//       return;
//     }
//       
//    if(convertPosition(cstart) >= csize)
//       return;
//    QProgressDialog* progress = 0;
//    if (showProgress) {
//       QString label(QWidget::tr("create peakfile for "));
//       label += basename();
//       progress = new QProgressDialog(label,
//                                      QString::null, 0, csize, 0);
//       progress->setMinimumDuration(0);
//       progress->show();
//    }
//    
//    const int srcChannels = channels();
//    
//    float data[srcChannels][cacheMag];
//    float* fp[srcChannels];
//    for (int k = 0; k < srcChannels; ++k)
//       fp[k] = &data[k][0];
//    int interval = (csize - convertPosition(cstart)) / 10;
// 
//    if(!interval)
//       interval = 1;
//    for (int i = convertPosition(cstart); i < csize; i++) {
//       if (showProgress && ((i % interval) == 0))
//          progress->setValue(i);
//       seek(i * cacheMag, 0);
//       // Reset the converter. Its current state is meaningless now.
//       _audConv->reset();
//       //read(channels(), fp, cacheMag);
//       readConverted(srcChannels, fp, cacheMag);
//       for (int ch = 0; ch < srcChannels; ++ch) {
//          float rms = 0.0;
//          cache[ch][i].peak = 0;
//          for (int n = 0; n < cacheMag; n++) {
//             float fd = data[ch][n];
//             rms += fd * fd;
//             int idata = int(fd * 255.0);
//             if (idata < 0)
//                idata = -idata;
//             if (cache[ch][i].peak < idata)
//                cache[ch][i].peak = idata;
//          }
//          // amplify rms value +12dB
//          int rmsValue = int((sqrt(rms/cacheMag) * 255.0));
//          if (rmsValue > 255)
//             rmsValue = 255;
//          cache[ch][i].rms = rmsValue;
//       }
//    }
//    if (showProgress)
//       progress->setValue(csize);
//    if(bWrite)
//       writeCache(path);
//    if (showProgress)
//       delete progress;
// 
// }

//---------------------------------------------------------
//   readCache
//---------------------------------------------------------

void SndFile::readCache(const QString& path, bool showProgress)
{
   if (cache) {
      delete[] cache;
   }
   if (samples() == 0)
      return;

   const int srcChannels = channels();
   
   csize = (samples() + cacheMag - 1)/cacheMag;
   cache = new SampleVtype[srcChannels];
   for (int ch = 0; ch < srcChannels; ++ch)
   {
      cache [ch].resize(csize);
   }

   FILE* cfile = fopen(path.toLocal8Bit().constData(), "r");
   if (cfile) {
      for (int ch = 0; ch < srcChannels; ++ch)
         fread(&cache[ch] [0], csize * sizeof(SampleV), 1, cfile);
      fclose(cfile);
      return;
   }


   createCache(path, showProgress, true);
}

//---------------------------------------------------------
//   readCacheConverted
//---------------------------------------------------------

// void SndFile::readCacheConverted(const QString& path, bool showProgress)
// {
//    if(!sampleRateDiffers() || !_audConvUI)
//    {
//      readCache(path, showProgress);
//      return;
//    }
//       
//    if (cache) {
//       delete[] cache;
//    }
//    if (samples() == 0)
//       return;
// 
//    const int srcChannels = channels();
//    
//    csize = (convertPosition(samples()) + cacheMag - 1)/cacheMag;
//    cache = new SampleVtype[srcChannels];
//    for (int ch = 0; ch < srcChannels; ++ch)
//    {
//       cache [ch].resize(csize);
//    }
// 
//    FILE* cfile = fopen(path.toLocal8Bit().constData(), "r");
//    if (cfile) {
//       for (int ch = 0; ch < srcChannels; ++ch)
//          fread(&cache[ch] [0], csize * sizeof(SampleV), 1, cfile);
//       fclose(cfile);
//       return;
//    }
// 
// 
//    createCacheConverted(path, showProgress, true);
// }

//---------------------------------------------------------
//   writeCache
//---------------------------------------------------------

void SndFile::writeCache(const QString& path)
      {
      FILE* cfile = fopen(path.toLocal8Bit().constData(), "w");
      if (cfile == 0)
            return;
      const int srcChannels = channels();
      for (int ch = 0; ch < srcChannels; ++ch)
            fwrite(&cache[ch] [0], csize * sizeof(SampleV), 1, cfile);
      fclose(cfile);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void SndFile::read(SampleV* s, int mag, unsigned pos, bool overwrite, bool allowSeek)
      {
      const int srcChannels = channels();
      
      if(overwrite)
        for (int ch = 0; ch < srcChannels; ++ch) {
            s[ch].peak = 0;
            s[ch].rms = 0;
            }

      // only check pos if seek is allowed
      // for realtime drawing of the wave
      // seek may cause writing errors
      if (allowSeek && pos > samples())
            return;

      if (mag < cacheMag) {
            float data[srcChannels][mag];
            float* fp[srcChannels];
            for (int i = 0; i < srcChannels; ++i)
                  fp[i] = &data[i][0];

            sf_count_t ret = 0;
            if(sfUI)
              ret = sf_seek(sfUI, pos, SEEK_SET | SFM_READ);
            else
              ret = sf_seek(sf, pos, SEEK_SET | SFM_READ);
            
            if(ret == -1)
              return;
            {
            const int dstChannels = sfinfo.channels;
            const size_t n        = mag;
            float** dst     = fp;
            float buffer[n * dstChannels];

            size_t rn = 0;
            if(sfUI)
              rn = sf_readf_float(sfUI, buffer, n);
            else
              rn = sf_readf_float(sf, buffer, n);
            
            if(rn != n)
              return;
            float* src = buffer;

            if (srcChannels == dstChannels) {
                  for (size_t i = 0; i < rn; ++i) {
                        for (int ch = 0; ch < srcChannels; ++ch)
                              *(dst[ch]+i) = *src++;
                        }
                  }
            else if ((srcChannels == 1) && (dstChannels == 2)) {
                  // stereo to mono
                  for (size_t i = 0; i < rn; ++i)
                        *(dst[0] + i) = src[i + i] + src[i + i + 1];
                  }
            else if ((srcChannels == 2) && (dstChannels == 1)) {
                  // mono to stereo
                  for (size_t i = 0; i < rn; ++i) {
                        float data = *src++;
                        *(dst[0]+i) = data;
                        *(dst[1]+i) = data;
                        }
                  }
            }

            for (int ch = 0; ch < srcChannels; ++ch) {

                  if(overwrite)
                    s[ch].peak = 0;

                  float rms = 0.0;
                  for (int i = 0; i < mag; i++) {
                        float fd = data[ch][i];
                        rms += fd;
                        int idata = int(fd * 255.0);
                        if (idata < 0)
                              idata = -idata;
                        if (s[ch].peak < idata)
                              s[ch].peak = idata;
                        }

                    s[ch].rms = 0;    // TODO rms / mag;
                  }
            }
      else {
            mag /= cacheMag;
            int rest = csize - (pos/cacheMag);
            int end  = mag;
            if (rest < mag)
                  end = rest;

            for (int ch = 0; ch < srcChannels; ++ch) {
                  int rms = 0;
                  int off = pos/cacheMag;
                  for (int offset = off; offset < off+end; offset++) {
                        rms += cache[ch][offset].rms;
                        if (s[ch].peak < cache[ch][offset].peak)
                              s[ch].peak = cache[ch][offset].peak;
                              }

                  if(overwrite)
                    s[ch].rms = rms / mag;

                  else
                    s[ch].rms += rms / mag;
                  }
            }
      }

// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   readConverted
//---------------------------------------------------------

void SndFile::readConverted(SampleV* s, int mag, sf_count_t pos, bool overwrite, bool allowSeek)
      {
      if(!(_staticAudioConverterUI && _staticAudioConverterUI->isValid() &&
          (((sampleRateDiffers() || isResampled()) && (_staticAudioConverterUI->capabilities() & AudioConverter::SampleRate)) ||
           (isStretched() && (_staticAudioConverterUI->capabilities() & AudioConverter::Stretch))) ))
      {
        read(s, mag, pos, overwrite, allowSeek);
        return;
      }
        
      const int srcChannels = channels();
        
      if(overwrite)
        for (int ch = 0; ch < srcChannels; ++ch) {
            s[ch].peak = 0;
            s[ch].rms = 0;
            }

      // only check pos if seek is allowed
      // for realtime drawing of the wave
      // seek may cause writing errors
// REMOVE Tim. samplerate. Changed.
//       if (allowSeek && pos > samples())
//             return;
// Removed. NOTE Seeking is now done only once in the graphic code.
//       if(allowSeek && convertPosition(pos) > convertPosition(samples()))
//             return;

      if (mag < cacheMag) {
            float data[srcChannels][mag];
            float* fp[srcChannels];
            for (int i = 0; i < srcChannels; ++i)
                  fp[i] = &data[i][0];

            sf_count_t ret = 0;
// REMOVE Tim. samplerate. Changed.
//             if(sfUI)
//               ret = sf_seek(sfUI, pos, SEEK_SET | SFM_READ);
//             else
//               ret = sf_seek(sf, pos, SEEK_SET | SFM_READ);

// Removed. NOTE Seeking is now done only once in the graphic code.
//             if(sfUI)
//             {
//               ret = sf_seek(sfUI, convertPosition(pos), SEEK_SET | SFM_READ);
//               // Reset the converter. Its current state is meaningless now.
//               _staticAudioConverterUI->reset();
//             }
//             else
//             {
//               ret = sf_seek(sf, convertPosition(pos), SEEK_SET | SFM_READ);
//               // Reset the converter. Its current state is meaningless now.
//               _staticAudioConverter->reset();
//             }
//
//             // Reset the converter. Its current state is meaningless now.
//             //_audConvUI->reset();
//
//             if(ret == -1)
//               return;
            
            
            //{
//             const int srcChannels = channels();
//             int dstChannels = sfinfo.channels;
            const sf_count_t n        = mag;
//             float** dst     = fp;
//             float buffer[n * dstChannels];

//             sf_count_t rn = 0;
// REMOVE Tim. samplerate. Changed.
//             if(sfUI)
//               rn = sf_readf_float(sfUI, buffer, n);
//             else
//               rn = sf_readf_float(sf, buffer, n);
            if(sfUI)
              //rn = sf_readf_float(sfUI, buffer, n);
              //rn = _audConvUI->process(this, buffer, srcChannels, n, true);
              //rn = _audConvUI->process(this, dst, srcChannels, n, true);
              //rn = _audConvUI->process(this, sfUI,  dst, srcChannels, n, true);
              //rn = _audConvUI->process(this, sfUI,  fp, srcChannels, n, true);
              ret = _staticAudioConverterUI->process(this, sfUI, pos, fp, srcChannels, n, true);
            else
              //rn = sf_readf_float(sf, buffer, n);
              //rn = _audConv->process(this, buffer, srcChannels, n, true);
              //rn = _audConv->process(this, dst, srcChannels, n, true);
              //rn = _audConv->process(this, sf, dst, srcChannels, n, true);
              //rn = _audConv->process(this, sf, fp, srcChannels, n, true);
              ret = _staticAudioConverter->process(this, sf, pos, fp, srcChannels, n, true);
            
            //if(rn != n)
            if(ret != n)
              return;
//             float* src = buffer;
// 
//             if (srcChannels == dstChannels) {
//                   for (sf_count_t i = 0; i < rn; ++i) {
//                         for (int ch = 0; ch < srcChannels; ++ch)
//                               *(dst[ch]+i) = *src++;
//                         }
//                   }
//             else if ((srcChannels == 1) && (dstChannels == 2)) {
//                   // stereo to mono
//                   for (sf_count_t i = 0; i < rn; ++i)
//                         *(dst[0] + i) = src[i + i] + src[i + i + 1];
//                   }
//             else if ((srcChannels == 2) && (dstChannels == 1)) {
//                   // mono to stereo
//                   for (sf_count_t i = 0; i < rn; ++i) {
//                         float data = *src++;
//                         *(dst[0]+i) = data;
//                         *(dst[1]+i) = data;
//                         }
//                   }
            //}

//             for (unsigned ch = 0; ch < channels(); ++ch) {
            for (int ch = 0; ch < srcChannels; ++ch) {

                  if(overwrite)
                    s[ch].peak = 0;

                  float rms = 0.0;
                  for (int i = 0; i < mag; i++) {
                        float fd = data[ch][i];
                        rms += fd;
                        int idata = int(fd * 255.0);
                        if (idata < 0)
                              idata = -idata;
                        if (s[ch].peak < idata)
                              s[ch].peak = idata;
                        }

                    s[ch].rms = 0;    // TODO rms / mag;
                  }
            }
      else {
            mag /= cacheMag;
//             int rest = csize - (pos/cacheMag);
            sf_count_t rest = csize - (convertPosition(pos)/cacheMag);
//             int end  = mag;
            sf_count_t end  = mag;
            if (rest < mag)
                  end = rest;

            for (int ch = 0; ch < srcChannels; ++ch) {
                  int rms = 0;
//                   int off = pos/cacheMag;
                  sf_count_t off = convertPosition(pos)/cacheMag;
//                   for (int offset = off; offset < off+end; offset++) {
                  for (sf_count_t offset = off; offset < off+end; offset++) {
                        rms += cache[ch][offset].rms;
                        if (s[ch].peak < cache[ch][offset].peak)
                              s[ch].peak = cache[ch][offset].peak;
                              }

                  if(overwrite)
                    s[ch].rms = rms / mag;

                  else
                    s[ch].rms += rms / mag;
                  }
            }
      }

//---------------------------------------------------------
//   openWrite
//---------------------------------------------------------

bool SndFile::openWrite()
      {
      if (openFlag) {
            DEBUG_WAVE(stderr, "SndFile:: alread open\n");
            return false;
            }
  QString p = path();

      sf = sf_open(p.toLocal8Bit().constData(), SFM_RDWR, &sfinfo);
      sfUI = NULL;
      if (sf) {
            if(writeBuffer)
              delete [] writeBuffer;
            writeBuffer = new float [writeSegSize * std::max(2, sfinfo.channels)];
            openFlag  = true;
            writeFlag = true;
            QString cacheName = finfo->absolutePath() +
               QString("/") + finfo->completeBaseName() + QString(".wca");
            readCache(cacheName, true);
            }
      return !sf;
      }

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SndFile::close()
      {
      //DEBUG_WAVE(stderr, "SndFile::close this:%p\n", this);
      INFO_WAVE(stderr, "SndFile::close this:%p\n", this); // REMOVE Tim. samplerate. Added. TESTING Change back to DEBUG_WAVE!
      if (!openFlag) {
            DEBUG_WAVE(stderr, "SndFile:: alread closed\n");
            return;
            }
      if(int err = sf_close(sf))
        ERROR_WAVE(stderr, "SndFile::close Error:%d on sf_close(sf:%p)\n", err, sf);
      else
        sf = NULL;
      if (sfUI)
      {
            if(int err = sf_close(sfUI))
              ERROR_WAVE(stderr, "SndFile::close Error:%d on sf_close(sfUI:%p)\n", err, sfUI);
            else
              sfUI = NULL;
      }
      openFlag = false;
      
      // REMOVE Tim. samplerate. Added.
//       if(_staticAudioConverter)
//         _staticAudioConverter = AudioConverter::release(_staticAudioConverter);
//       if(_staticAudioConverterUI)
//         _staticAudioConverterUI = AudioConverter::release(_staticAudioConverterUI);
//       if(_dynamicAudioConverter)
//         _dynamicAudioConverter = AudioConverter::release(_dynamicAudioConverter);
//       if(_dynamicAudioConverterUI)
//         _dynamicAudioConverterUI = AudioConverter::release(_dynamicAudioConverterUI);
      if(_staticAudioConverter)
      {
        delete _staticAudioConverter;
        _staticAudioConverter = NULL;
      }
      if(_staticAudioConverterUI)
      {
        delete _staticAudioConverterUI;
        _staticAudioConverterUI = NULL;
      }
      if(_dynamicAudioConverter)
      {
        delete _dynamicAudioConverter;
        _dynamicAudioConverter = NULL;
      }
      if(_dynamicAudioConverterUI)
      {
        delete _dynamicAudioConverterUI;
        _dynamicAudioConverterUI = NULL;
      }

      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void SndFile::remove()
      {
      if (openFlag)
            close();
      QFile::remove(finfo->filePath());
      }

QString SndFile::basename() const
      {
      return finfo->completeBaseName();
      }

QString SndFile::path() const
      {
      return finfo->filePath();
      }

QString SndFile::canonicalPath() const
      {
      return finfo->canonicalFilePath();
      }

QString SndFile::dirPath() const
      {
      return finfo->absolutePath();
      }

QString SndFile::canonicalDirPath() const
      {
      return finfo->canonicalPath();
      }

QString SndFile::name() const
      {
      return finfo->fileName();
      }

      
//---------------------------------------------------------
//   sampleRateRatio
//---------------------------------------------------------

double SndFile::sampleRateRatio() const
{
  //if(sampleRateDiffers())
    return (double)sfinfo.samplerate / (double)MusEGlobal::sampleRate;
  //return 1.0;
}

//---------------------------------------------------------
//   sampleRateDiffers
//---------------------------------------------------------

bool SndFile::sampleRateDiffers() const
{
  return sfinfo.samplerate != MusEGlobal::sampleRate;
}

bool SndFile::isStretched() const 
{ 
  return _stretchList->isStretched();
}

bool SndFile::isResampled() const 
{ 
  return _stretchList->isResampled();
}

//---------------------------------------------------------
//   convertPosition
//---------------------------------------------------------

sf_count_t SndFile::convertPosition(sf_count_t pos) const
{
  //return sf_count_t((double)pos * sampleRateRatio()) + 1  // From MusE-2 file converter.
  //return floor(((double)pos * sampleRateRatio()));          // From simplesynth.
//   return double(pos) * sampleRateRatio();
  
//   double new_pos = pos;
//   
//   //double samplerate_ratio = 1.0;
//   if(_staticAudioConverter && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate))
//     new_pos *= sampleRateRatio();
// 
//   //double stretch_ratio = 1.0;
//   //if(_staticAudioConverter && _stretchList && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))
//   if(_staticAudioConverter && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))
//     //stretch_ratio = _stretchList->stretch();
//     new_pos = _stretchList->stretch(new_pos);
//   //return double(pos) * sampleRateRatio();
  
  //
  // Must do stretch first, then samplerate conversion.
  //
//   double new_pos = pos;
//   if(_staticAudioConverter && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))
// //     new_pos = _stretchList->stretch(pos);
//     new_pos = _stretchList->unSquish(pos);
//     //new_pos = pos;
//   if(_staticAudioConverter && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate))
//     new_pos *= sampleRateRatio();

  double new_pos = pos;
  if(_staticAudioConverter)
  {
    int type = 0;
    if(_staticAudioConverter->capabilities() & AudioConverter::Stretch)
      type |= StretchListItem::StretchEvent;
    if(_staticAudioConverter->capabilities() & AudioConverter::SampleRate)
      type |= StretchListItem::SamplerateEvent;

//     new_pos = _stretchList->stretch(pos);
    if(type != 0)
      new_pos = _stretchList->unSquish(pos, type);
    //new_pos = pos;

    if(_staticAudioConverter->capabilities() & AudioConverter::SampleRate)
      new_pos *= sampleRateRatio();
  }

  return new_pos;
}

//---------------------------------------------------------
//   samples
//---------------------------------------------------------

sf_count_t SndFile::samples() const
      {
      if (!writeFlag) // if file is read only sfinfo is reliable
          return sfinfo.frames;
      SNDFILE* sfPtr = sf;
      if (sfUI)
        sfPtr = sfUI;
      sf_count_t curPos = sf_seek(sfPtr, 0, SEEK_CUR | SFM_READ);
      sf_count_t frames = sf_seek(sfPtr, 0, SEEK_END | SFM_READ);
      sf_seek(sfPtr, curPos, SEEK_SET | SFM_READ);
      return frames;
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

int SndFile::channels() const
      {
      return sfinfo.channels;
      }

int SndFile::samplerate() const
      {
      return sfinfo.samplerate;
      }

int SndFile::format() const
      {
      return sfinfo.format;
      }

void SndFile::setFormat(int fmt, int ch, int rate)
      {
      sfinfo.samplerate = rate;
      sfinfo.channels   = ch;
      sfinfo.format     = fmt;
      sfinfo.seekable   = true;
      sfinfo.frames     = 0;
      }

//---------------------------------------------------------
//   readWithHeap
//   not as realtime friendly but can retrieve bigger data
//---------------------------------------------------------
size_t SndFile::readWithHeap(int srcChannels, float** dst, size_t n, bool overwrite)
      {
      float *buffer = new float[n * sfinfo.channels];
      int rn = readInternal(srcChannels,dst,n,overwrite, buffer);
      delete[] buffer;
      return rn;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------
size_t SndFile::read(int srcChannels, float** dst, size_t n, bool overwrite)
      {
      float buffer[n * sfinfo.channels];
      int rn = readInternal(srcChannels,dst,n,overwrite, buffer);
      return rn;
      }

size_t SndFile::readInternal(int srcChannels, float** dst, size_t n, bool overwrite, float *buffer)
{
      size_t rn = sf_readf_float(sf, buffer, n);

      float* src      = buffer;
      int dstChannels = sfinfo.channels;
      if (srcChannels == dstChannels) {
            if(overwrite)
              for (size_t i = 0; i < rn; ++i) {
                  for (int ch = 0; ch < srcChannels; ++ch)
                        *(dst[ch]+i) = *src++;
                  }
              else
              for (size_t i = 0; i < rn; ++i) {
                  for (int ch = 0; ch < srcChannels; ++ch)
                        *(dst[ch]+i) += *src++;
                  }
            }
      else if ((srcChannels == 1) && (dstChannels == 2)) {
            // stereo to mono
            if(overwrite)
              for (size_t i = 0; i < rn; ++i)
                  *(dst[0] + i) = src[i + i] + src[i + i + 1];
            else
              for (size_t i = 0; i < rn; ++i)
                  *(dst[0] + i) += src[i + i] + src[i + i + 1];
            }
      else if ((srcChannels == 2) && (dstChannels == 1)) {
            // mono to stereo
            if(overwrite)
              for (size_t i = 0; i < rn; ++i) {
                  float data = *src++;
                  *(dst[0]+i) = data;
                  *(dst[1]+i) = data;
                  }
            else
              for (size_t i = 0; i < rn; ++i) {
                  float data = *src++;
                  *(dst[0]+i) += data;
                  *(dst[1]+i) += data;
                  }
            }
      else {
            ERROR_WAVE(stderr, "SndFile:read channel mismatch %d -> %d\n",
               srcChannels, dstChannels);
            }

      return rn;

}

sf_count_t SndFile::readConverted(sf_count_t pos, int srcChannels, float** buffer, sf_count_t frames, bool overwrite)
{
  if(_staticAudioConverter && _staticAudioConverter->isValid() &&
     (((sampleRateDiffers() || isResampled()) && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate)) ||
      (isStretched() && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))) )
    return _staticAudioConverter->process(this, sf, pos, buffer, srcChannels, frames, overwrite);
  return read(srcChannels, buffer, frames, overwrite);
}

//---------------------------------------------------------
//   write
//
//   A hardcoded limiter was added that limits the output at 0.99/-0.99
//   libsndfile handles signal betwee -1.0/1.0 with current setting
//   outside these values there will be heavy distortion
//
//---------------------------------------------------------

size_t SndFile::write(int srcChannels, float** src, size_t n)
{
   size_t wrFrames = 0;

   if(n <= writeSegSize)
       wrFrames = realWrite(srcChannels, src, n);
   else
   {
      while(1)
      {
         size_t sz = (n - wrFrames) < writeSegSize ? (n - wrFrames) : writeSegSize;
         size_t nrWrote = realWrite(srcChannels, src, sz, wrFrames);
         if(nrWrote == 0) // Nothing written?
           break;
         wrFrames += nrWrote;
         if(wrFrames >= n)
           break;
      }
   }
   return wrFrames;
}

size_t SndFile::realWrite(int srcChannels, float** src, size_t n, size_t offs)
{
   int dstChannels = sfinfo.channels;
   float *dst      = writeBuffer;

   size_t iStart = offs;
   size_t iEnd = offs + n;

   const float limitValue=0.9999;


   if (srcChannels == dstChannels) {
      for (size_t i = iStart; i < iEnd; ++i) {
         for (int ch = 0; ch < dstChannels; ++ch)
            if (*(src[ch]+i) > 0)
               *dst++ = *(src[ch]+i) < limitValue ? *(src[ch]+i) : limitValue;
            else
               *dst++ = *(src[ch]+i) > -limitValue ? *(src[ch]+i) : -limitValue;
      }
   }
   else if ((srcChannels == 1) && (dstChannels == 2)) {
      // mono to stereo
      for (size_t i = iStart; i < iEnd; ++i) {
         float data =  *(src[0]+i);
         if (data > 0) {
            *dst++ = data < limitValue ? data : limitValue;
            *dst++ = data < limitValue ? data : limitValue;
         }
         else {
            *dst++ = data > -limitValue ? data : -limitValue;
            *dst++ = data > -limitValue ? data : -limitValue;
         }
      }
   }
   else if ((srcChannels == 2) && (dstChannels == 1)) {
      // stereo to mono
      for (size_t i = iStart; i < iEnd; ++i) {
         if (*(src[0]+i) + *(src[1]+i) > 0)
            *dst++ = (*(src[0]+i) + *(src[1]+i)) < limitValue ? (*(src[0]+i) + *(src[1]+i)) : limitValue;
         else
            *dst++ = (*(src[0]+i) + *(src[1]+i)) > -limitValue ? (*(src[0]+i) + *(src[1]+i)) : -limitValue;
      }
   }
   else {
      ERROR_WAVE(stderr, "SndFile:write channel mismatch %d -> %d\n",
             srcChannels, dstChannels);
      return 0;
   }
   int nbr = sf_writef_float(sf, writeBuffer, n) ;

   if(MusEGlobal::config.liveWaveUpdate)
   { //update cache
      if(!cache)
      {
         cache = new SampleVtype[sfinfo.channels];
         csize = 0;
      }
      sf_count_t cstart = (sfinfo.frames + cacheMag - 1) / cacheMag;
      sfinfo.frames += n;
      csize = (sfinfo.frames + cacheMag - 1) / cacheMag;
      for (int ch = 0; ch < sfinfo.channels; ++ch)
      {
         cache [ch].resize(csize);
      }

      for (int i = cstart; i < csize; i++)
      {
         for (int ch = 0; ch < sfinfo.channels; ++ch)
         {
            float rms = 0.0;
            cache[ch][i].peak = 0;
            for (int n = 0; n < cacheMag; n++)
            {
               //float fd = data[ch][n];
               float fd = writeBuffer [n * sfinfo.channels + ch];
               rms += fd * fd;
               int idata = int(fd * 255.0);
               if (idata < 0)
                  idata = -idata;
               if (cache[ch][i].peak < idata)
                  cache[ch][i].peak = idata;
            }
            // amplify rms value +12dB
            int rmsValue = int((sqrt(rms/cacheMag) * 255.0));
            if (rmsValue > 255)
               rmsValue = 255;
            cache[ch][i].rms = rmsValue;
         }
      }

   }

   return nbr;
}

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

// REMOVE Tim. samplerate. Changed.
//off_t SndFile::seek(off_t frames, int whence)
sf_count_t SndFile::seek(sf_count_t frames, int whence)
      {
      return sf_seek(sf, frames, whence);
      }

// REMOVE Tim. samplerate. Added.
sf_count_t SndFile::seekUI(sf_count_t frames, int whence)
{
  sf_count_t rn = 0;
  if(sfUI)
    rn = sf_seek(sfUI, frames, whence);
  else if(sf)
    rn = sf_seek(sf, frames, whence);
  return rn;
}

// REMOVE Tim. samplerate. Added.
sf_count_t SndFile::seekUIConverted(sf_count_t frames, int whence)
{
  sf_count_t rn = 0;
  if(sfUI)
  {
    rn = sf_seek(sfUI, convertPosition(frames), whence);
    // Reset the converter. Its current state is meaningless now.
    if(_staticAudioConverterUI)
      _staticAudioConverterUI->reset();
  }
  else if(sf)
  {
    rn = sf_seek(sf, convertPosition(frames), whence);
    // Reset the converter. Its current state is meaningless now.
    if(_staticAudioConverter)
      _staticAudioConverter->reset();
  }
  return rn;
}

// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   seekConverted
//   The offset is the offset into the sound file and is NOT converted.
//---------------------------------------------------------

sf_count_t SndFile::seekConverted(sf_count_t frames, int whence, int offset)
      {
      if(_staticAudioConverter && _staticAudioConverter->isValid() &&
         (((sampleRateDiffers() || isResampled()) && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate)) ||
          (isStretched() && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))) )
        return _staticAudioConverter->seekAudio(this, frames, offset);
      return seek(frames + offset, whence);
      }

//---------------------------------------------------------
//   strerror
//---------------------------------------------------------

QString SndFile::strerror() const
      {
      char buffer[128];
      buffer[0] = 0;
      sf_error_str(sf, buffer, 128);
      return QString(buffer);
}

//---------------------------------------------------------
//   search
//---------------------------------------------------------

SndFile* SndFileList::search(const QString& name)
      {
      for (iSndFile i = begin(); i != end(); ++i) {
            if ((*i)->path() == name)
                  return *i;
            }
      return NULL;
      }

//---------------------------------------------------------
//   getWave
//   If audioConverterSettings and stretchList are given, they are assigned.
//---------------------------------------------------------

SndFileR getWave(const QString& inName, bool readOnlyFlag, bool openFlag, bool showErrorBox, 
                 const AudioConverterSettingsGroup* audioConverterSettings, const StretchList* stretchList)
      {
      QString name = inName;

      if (QFileInfo(name).isRelative()) {
            name = MusEGlobal::museProject + QString("/") + name;
            }
      else {
            if (!QFile::exists(name)) {
                  if (QFile::exists(MusEGlobal::museProject + QString("/") + name)) {
                        name = MusEGlobal::museProject + QString("/") + name;
                        }
                  }
            }

      // only open one instance of wave file
      // REMOVE Tim. Sharing. Changed. Allow multiple instances. For testing only so far.
      //SndFile* f = SndFile::sndFiles.search(name);
      SndFile* f = NULL;
      //if (!f)
      //{
            if (!QFile::exists(name)) {
                  ERROR_WAVE(stderr, "wave file <%s> not found\n",
                     name.toLocal8Bit().constData());
                  return NULL;
                  }
            f = new SndFile(name);

            // REMOVE Tim. samplerate. Added.
            // Assign audio converter settings if given.
            if(audioConverterSettings)
              f->audioConverterSettings()->assign(*audioConverterSettings); 
            // Assign stretch list if given.
            if(stretchList)
              *f->stretchList() = *stretchList;
              
            if(openFlag)
            {
              bool error;
              if (readOnlyFlag)
                    error = f->openRead();
              else {
                    error = f->openWrite();
                    // if peak cache is older than wave file we reaquire the cache
                    QFileInfo wavinfo(name);
                    QString cacheName = wavinfo.absolutePath() + QString("/") + wavinfo.completeBaseName() + QString(".wca");
                    QFileInfo wcainfo(cacheName);
                    if (!wcainfo.exists() || wcainfo.lastModified() < wavinfo.lastModified()) {
                          QFile(cacheName).remove();
                          f->readCache(cacheName,true);
                          }

              }
              if (error) {
                    ERROR_WAVE(stderr, "open wave file(%s) for %s failed: %s\n",
                      name.toLocal8Bit().constData(),
                      readOnlyFlag ? "writing" : "reading",
                      f->strerror().toLocal8Bit().constData());
                      if(showErrorBox)
                        QMessageBox::critical(NULL, "MusE import error.",
                                        "MusE failed to import the file.\n"
                                        "Possibly this wasn't a sound file?\n"
                                        "If it was check the permissions, MusE\n"
                                        "sometimes requires write access to the file.");

                    delete f;
                    f = NULL;
                    }
              }
//             }
//       else {
//               if(openFlag)
//               {
//                 if (!readOnlyFlag && ! f->isWritable()) {
//                       if (f->isOpen())
//                             f->close();
//                       f->openWrite();
//                       }
//                 else {
//                       // if peak cache is older than wave file we reaquire the cache
//                       QFileInfo wavinfo(name);
//                       QString cacheName = wavinfo.absolutePath() + QString("/") + wavinfo.completeBaseName() + QString(".wca");
//                       QFileInfo wcainfo(cacheName);
//                       if (!wcainfo.exists() || wcainfo.lastModified() < wavinfo.lastModified()) {
//                             QFile(cacheName).remove();
//                             f->readCache(cacheName,true);
//                             }
//
//                       }
//               }
//             }
      return f;
      }

//---------------------------------------------------------
//   applyUndoFile
//---------------------------------------------------------
void SndFile::applyUndoFile(const Event& original, const QString* tmpfile, unsigned startframe, unsigned endframe)
      {
      // This one is called on both undo and redo of a wavfile
      // For redo to be called, undo must have been called first, and we don't store both the original data and the modified data in separate
      // files. Thus, each time this function is called the data in the "original"-file will be written to the tmpfile, after the data
      // from the tmpfile has been applied.
      //
      // F.ex. if mute has been made on part of a wavfile, the unmuted data is stored in the tmpfile when
      // the undo operation occurs. The unmuted data is then written back to the original file, and the mute data will be
      // put in the tmpfile, and when redo is eventually called the data is switched again (causing the muted data to be written to the "original"
      // file. The data is merely switched.

      if (original.empty()) {
            ERROR_WAVE(stderr, "SndFile::applyUndoFile: Internal error: original event is empty - Aborting\n");
            return;
            }

      SndFileR orig = original.sndFile();

      if (orig.isNull()) {
            ERROR_WAVE(stderr, "SndFile::applyUndoFile: Internal error: original sound file is NULL - Aborting\n");
            return;
            }
      if (orig.canonicalPath().isEmpty()) {
            ERROR_WAVE(stderr, "SndFile::applyUndoFile: Error: Original sound file name is empty - Aborting\n");
            return;
            }

      if (!orig.isOpen()) {
            if (orig.openRead()) {
                  ERROR_WAVE(stderr, "Cannot open original file %s for reading - cannot undo! Aborting\n", orig.canonicalPath().toLocal8Bit().constData());
                  return;
                  }
            }

      SndFile tmp  = SndFile(*tmpfile);
      if (!tmp.isOpen()) {
            if (tmp.openRead()) {
                  ERROR_WAVE(stderr, "Could not open temporary file %s for writing - cannot undo! Aborting\n", tmpfile->toLocal8Bit().constData());
                  return;
                  }
            }

      MusEGlobal::audio->msgIdle(true);
      tmp.setFormat(orig.format(), orig.channels(), orig.samplerate());

      // Read data in original file to memory before applying tmpfile to original
      unsigned file_channels = orig.channels();
      unsigned tmpdatalen = endframe - startframe;
      float*   data2beoverwritten[file_channels];

      for (unsigned i=0; i<file_channels; i++) {
            data2beoverwritten[i] = new float[tmpdatalen];
            }
      orig.seek(startframe, 0);
      orig.readWithHeap(file_channels, data2beoverwritten, tmpdatalen);

      orig.close();

      // Read data from temporary file to memory
      float* tmpfiledata[file_channels];
      for (unsigned i=0; i<file_channels; i++) {
            tmpfiledata[i] = new float[tmpdatalen];
            }
      tmp.seek(0, 0);
      tmp.readWithHeap(file_channels, tmpfiledata, tmpdatalen);
      tmp.close();

      // Write temporary data to original file:
      if (orig.openWrite()) {
            ERROR_WAVE(stderr, "Cannot open orig for write - aborting.\n");
            return;
            }

      orig.seek(startframe, 0);
      orig.write(file_channels, tmpfiledata, tmpdatalen);

      // Delete dataholder for temporary file
      for (unsigned i=0; i<file_channels; i++) {
            delete[] tmpfiledata[i];
            }

      // Write the overwritten data to the tmpfile
      if (tmp.openWrite()) {
            ERROR_WAVE(stderr, "Cannot open tmpfile for writing - redo operation of this file won't be possible. Aborting.\n");
            MusEGlobal::audio->msgIdle(false);
            return;
            }
      tmp.seek(0, 0);
      tmp.write(file_channels, data2beoverwritten, tmpdatalen);
      tmp.close();

      // Delete dataholder for replaced original file
      for (unsigned i=0; i<file_channels; i++) {
            delete[] data2beoverwritten[i];
            }

      orig.close();
      orig.openRead();
      orig.update();
      MusEGlobal::audio->msgIdle(false);
      }

//---------------------------------------------------------
//   checkCopyOnWrite
//---------------------------------------------------------

bool SndFile::checkCopyOnWrite()
{
  QString path_this = canonicalPath();
  if(path_this.isEmpty())
    return false;

  bool fwrite = finfo->isWritable();

  // No exceptions: Even if this wave event is a clone, if it ain't writeable we gotta copy the wave.
  if(!fwrite)
    return true;

  // Count the number of unique part wave events (including possibly this one) using this file.
  // Not much choice but to search all active wave events - the sndfile ref count is not the solution for this...
  int use_count = 0;
  EventID_t id = MUSE_INVALID_EVENT_ID;
  Part* part = NULL;
  WaveTrackList* wtl = MusEGlobal::song->waves();
  for(ciTrack it = wtl->begin(); it != wtl->end(); ++it)
  {
    PartList* pl = (*it)->parts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
    {
      Part* p = ip->second;
      const EventList& el = p->events();
//       const EventList& el = ip->second->events();
//       // We are looking for active independent non-clone parts
//       if(ip->second->hasClones())
//         continue;
      for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
      {
        if(ie->second.type() != Wave)
          continue;
        const Event& ev = ie->second;
        if(ev.empty() || ev.id() == MUSE_INVALID_EVENT_ID)
          continue;
        const SndFileR sf = ev.sndFile();
        if(sf.isNull())
          continue;
        QString path = sf.canonicalPath();
        if(path.isEmpty())
          continue;
        if(path == path_this)
        {
          // Ignore clones of an already found event.
          if(ev.id() == id)
          {
            // Double check.
            if(part && !p->isCloneOf(part))
              ERROR_WAVE(stderr, "SndFile::checkCopyOnWrite() Error: Two event ids are the same:%d but their parts:%p, %p are not clones!\n", (int)id, p, part);
            continue;
          }
          part = p;
          id = ev.id();
          ++use_count;
        }
        // If more than one unique part wave event is using the file, signify that the caller should make a copy of it.
        if(use_count > 1)
          return true;
      }
    }
  }

  return false;
}

// DELETETHIS 170
#if 0
//---------------------------------------------------------
//   Clip
//---------------------------------------------------------

ClipBase::ClipBase(const SndFileR& file, int start, int l)
   : f(file)
      {
      refCount = 0;
      for (int i = 1; true; ++i) {
            _name.sprintf("%s.%d", f.basename().toLocal8Bit().constData(), i);
            ciClip ic = waveClips->begin();
            for (; ic != waveClips->end(); ++ic) {
                  if ((*ic)->name() == _name)
                        break;
                  }
            if (ic == waveClips->end())
                  break;
            // try another name
            }
      _spos   = start;
      len     = l;
      deleted = false;
      lrefs   = 0;
      waveClips->add(this);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void ClipBase::read(unsigned srcOffset, float** buffer, int channel, unsigned n)
      {
      if (f.isNull())
            return;
      f.seek(srcOffset + _spos, 0);
      f.read(channel, buffer, n);
      }

ClipBase::~ClipBase()
      {
      waveClips->remove(this);
      }

//---------------------------------------------------------
//   ClipList::write(level, xml)
//---------------------------------------------------------

void ClipList::write(int level, Xml& xml) const
      {
      for (ciClip i = begin(); i != end(); ++i) {
            ClipBase* clip = *i;
            // only write visible clips
            if (clip->references())
                  (*i)->write(level, xml);
            }
      }

//---------------------------------------------------------
//   ClipBase::write(level, xml)
//---------------------------------------------------------

void ClipBase::write(int level, Xml& xml) const
      {
      xml.tag(level++, "clip");
      QString path = f.dirPath();

      //
      // waves in the project directory are stored
      // with relative path name, others with absolute path
      //
      if (path == MusEGlobal::museProject)
            xml.strTag(level, "file", f.name());
      else
            xml.strTag(level, "file", f.path());

      xml.strTag(level, "name", _name);
      xml.intTag(level, "tick", _spos);
      xml.intTag(level, "len", len);
      xml.etag(level, "clip");
      }

//---------------------------------------------------------
//   ClipBase::read
//---------------------------------------------------------

ClipBase* readClip(Xml& xml)
      {
      SndFileR f = 0;
      QString name;
      unsigned spos = 0;
      int len = 0;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return 0;
                  case Xml::TagStart:
                        if (tag == "file")
                              f = getWave(xml.parse1(), false);
                        else if (tag == "name")
                              name = xml.parse1();
                        else if (tag == "tick")
                              spos = xml.parseInt();
                        else if (tag == "len")
                              len = xml.parseInt();
                        else
                              xml.unknown("Clip");
                        break;
                  case Xml::TagEnd:
                        if (tag == "clip") {
                              if (!f)
                                    printf("clip: file not found\n");
                              ClipBase* clip = new ClipBase(f, spos, len);
                              clip->setName(name);
                              return clip;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   search
//---------------------------------------------------------

Clip ClipList::search(const QString& name) const
      {
      for (ciClip i = begin(); i != end(); ++i)
            if ((*i)->name() == name)
                  return Clip(*i);
      fprintf(stderr, "ClipList: clip <%s> not found\n",
         name.toLocal8Bit().constData());
      return Clip();
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void ClipList::remove(ClipBase* clip)
      {
      for (iClip i = begin(); i != end(); ++i) {
            if (*i == clip) {
                  erase(i);
                  return;
                  }
            }
      printf("ClipList:remove: clip not found\n");
      }

//---------------------------------------------------------
//   idx
//---------------------------------------------------------

int ClipList::idx(const Clip& clip) const
      {
      int n = 0;
      for (ciClip i = begin(); i != end(); ++i, ++n) {
            if (clip == *i)
                  return n;
            }
      return -1;
      }
#endif

//---------------------------------------------------------
//   cmdAddRecordedWave
//---------------------------------------------------------

void Song::cmdAddRecordedWave(MusECore::WaveTrack* track, MusECore::Pos s, MusECore::Pos e, Undo& operations)
      {
      if (MusEGlobal::debugMsg)
          INFO_WAVE(stderr, "cmdAddRecordedWave - loopCount = %d, punchin = %d", MusEGlobal::audio->loopCount(), punchin());

      // Driver should now be in transport 'stop' mode and no longer pummping the recording wave fifo,
      //  but the fifo may not be empty yet, it's in the prefetch thread.
      // Wait a few seconds for the fifo to be empty, until it has been fully transferred to the
      //  track's recFile sndfile, which is done via Audio::process() sending periodic 'tick' messages
      //  to the prefetch thread to write its fifo to the sndfile, always UNLESS in stop or idle mode.
      // It now sends one final tick message at stop, so we /should/ have all our buffers available here.
      // This GUI thread is notified of the stop condition via the audio thread sending a message
      //  as soon as the state change is read from the driver.
      // NOTE: The fifo scheme is used only if NOT in transport freewheel mode where the data is directly
      //  written to the sndfile and therefore stops immediately when the transport stops and thus is
      //  safe to read here regardless of waiting.
      int tout = 100; // Ten seconds. Otherwise we gotta move on.
      while(track->recordFifoCount() != 0)
      {
        usleep(100000);
        --tout;
        if(tout == 0)
        {
          ERROR_WAVE(stderr, "Song::cmdAddRecordedWave: Error: Timeout waiting for _tempoFifo to empty! Count:%d\n", track->prefetchFifo()->getCount());
          break;
        }
      }

      // It should now be safe to work with the resultant sndfile here in the GUI thread.
      // No other thread should be touching it right now.
      MusECore::SndFileR f = track->recFile();
      if (f.isNull()) {
            ERROR_WAVE(stderr, "cmdAddRecordedWave: no snd file for track <%s>\n",
               track->name().toLocal8Bit().constData());
            return;
            }

      // If externally clocking (and therefore master was forced off),
      //  tempos may have been recorded. We really should temporarily force
      //  the master tempo map on in order to properly determine the ticks below.
      // Else internal clocking, the user decided to record either with or without
      //  master on, so let it be.
      // FIXME: We really should allow the master flag to be on at the same time as
      //  the external sync flag! AFAIR when external sync is on, no part of the app shall
      //  depend on the tempo map anyway, so it should not matter whether it's on or off.
      // If we do that, then we may be able to remove this section and user simply decides
      //  whether master is on/off, because we may be able to use the flag to determine
      //  whether to record external tempos at all, because we may want a switch for it!
      bool master_was_on = MusEGlobal::tempomap.masterFlag();
      if(MusEGlobal::extSyncFlag.value() && !master_was_on)
        MusEGlobal::tempomap.setMasterFlag(0, true);

      if((MusEGlobal::audio->loopCount() > 0 && s.tick() > lPos().tick()) || (punchin() && s.tick() < lPos().tick()))
        s.setTick(lPos().tick());
      // If we are looping, just set the end to the right marker, since we don't know how many loops have occurred.
      // (Fixed: Added Audio::loopCount)
      // Otherwise if punchout is on, limit the end to the right marker.
      if((MusEGlobal::audio->loopCount() > 0) || (punchout() && e.tick() > rPos().tick()) )
        e.setTick(rPos().tick());

      // No part to be created? Delete the rec sound file.
      if(s.frame() >= e.frame())
      {
        QString st = f->path();
        // The function which calls this function already does this immediately after. But do it here anyway.
        track->setRecFile(NULL); // upon "return", f is removed from the stack, the WaveTrack::_recFile's
                                 // counter has dropped by 2 and _recFile will probably deleted then
        remove(st.toLocal8Bit().constData());
        if(MusEGlobal::debugMsg)
          INFO_WAVE(stderr, "Song::cmdAddRecordedWave: remove file %s - startframe=%d endframe=%d\n", st.toLocal8Bit().constData(), s.frame(), e.frame());

        // Restore master flag.
        if(MusEGlobal::extSyncFlag.value() && !master_was_on)
          MusEGlobal::tempomap.setMasterFlag(0, false);

        return;
      }
// REMOVE Tim. Wave. Removed. Probably I should never have done this. It's more annoying than helpful. Look at it another way: Importing a wave DOES NOT do this.
//       // Round the start down using the Arranger part snap raster value.
//       int a_rast = MusEGlobal::song->arrangerRaster();
//       unsigned sframe = (a_rast == 1) ? s.frame() : Pos(MusEGlobal::sigmap.raster1(s.tick(), MusEGlobal::song->arrangerRaster())).frame();
//       // Round the end up using the Arranger part snap raster value.
//       unsigned eframe = (a_rast == 1) ? e.frame() : Pos(MusEGlobal::sigmap.raster2(e.tick(), MusEGlobal::song->arrangerRaster())).frame();
// //       unsigned etick = Pos(eframe, false).tick();
      unsigned sframe = s.frame();
      unsigned eframe = e.frame();

      // Done using master tempo map. Restore master flag.
      if(MusEGlobal::extSyncFlag.value() && !master_was_on)
        MusEGlobal::tempomap.setMasterFlag(0, false);

      f->update();

      MusECore::WavePart* part = new MusECore::WavePart(track);
      part->setFrame(sframe);
      part->setLenFrame(eframe - sframe);
      part->setName(track->name());

      // create Event
      MusECore::Event event(MusECore::Wave);
      event.setSndFile(f);
      // We are done with the _recFile member. Set to zero.
      track->setRecFile(0);

      event.setSpos(0);
      // Since the part start was snapped down, we must apply the difference so that the
      //  wave event tick lines up with when the user actually started recording.
      event.setFrame(s.frame() - sframe);
      // NO Can't use this. SF reports too long samples at first part recorded in sequence. See samples() - funny business with SEEK ?
      //event.setLenFrame(f.samples());
      event.setLenFrame(e.frame() - s.frame());
      part->addEvent(event);

      operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart, part));
      }

//---------------------------------------------------------
//   cmdChangeWave
//   called from GUI context
//---------------------------------------------------------
void Song::cmdChangeWave(const Event& original, QString tmpfile, unsigned sx, unsigned ex)
      {
      MusEGlobal::song->undoOp(UndoOp::ModifyClip, original, tmpfile, sx, ex);
      }

//---------------------------------------------------------
//   SndFileR
//---------------------------------------------------------

SndFileR::SndFileR(SndFile* _sf)
      {
      sf = _sf;
      if (sf)
            (sf->refCount)++;
      }

SndFileR::SndFileR(const SndFileR& ed)
      {
      sf = ed.sf;
      if (sf)
            (sf->refCount)++;
      }

//---------------------------------------------------------
//   operator=
//---------------------------------------------------------

SndFileR& SndFileR::operator=(SndFile* ptr)
{
      if (sf == ptr)
            return *this;
      if (sf && --(sf->refCount) == 0) {
            delete sf;
            }
      sf = ptr;
      if (sf)
            (sf->refCount)++;
      return *this;
}

SndFileR& SndFileR::operator=(const SndFileR& ed)
      {
      return operator=(ed.sf);
      }

//---------------------------------------------------------
//   ~SndFileR
//---------------------------------------------------------

SndFileR::~SndFileR()
      {
      *this=NULL; // decrease the refcounter, maybe delete
      }



} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   importAudio
//---------------------------------------------------------

void MusE::importWave()
{
   MusECore::Track* track = _arranger->curTrack();
   if (!track || track->type() != MusECore::Track::WAVE) {

      //just create new wave track and go on...
      if(MusEGlobal::song)
      {
         QAction act(MusEGlobal::song);
         act.setData(MusECore::Track::WAVE);
         track = MusEGlobal::song->addNewTrack(&act, NULL);
      }

      if(!track)
      {
         QMessageBox::critical(this, QString("MusE"),
                 tr("to import an audio file you have first to select"
                 "a wave track"));
               return;

      }

   }
   MusECore::AudioPreviewDialog afd(this, MusEGlobal::sampleRate);
   afd.setDirectory(MusEGlobal::lastWavePath);
   afd.setWindowTitle(tr("Import Audio File"));
   /*QString fn = afd.getOpenFileName(MusEGlobal::lastWavePath, MusEGlobal::audio_file_pattern, this,
         tr("Import Audio File"), 0);
*/
   if(afd.exec() == QFileDialog::Rejected)
   {
      return;
   }

   QStringList filenames = afd.selectedFiles();
   if(filenames.size() < 1)
   {
      return;
   }
   QString fn = filenames [0];

   if (!fn.isEmpty()) {
      MusEGlobal::lastWavePath = fn;
      importWaveToTrack(fn);
   }
}

//---------------------------------------------------------
//   importWaveToTrack
//---------------------------------------------------------

bool MusE::importWaveToTrack(QString& name, unsigned tick, MusECore::Track* track)
{
   if (track==NULL)
      track = (MusECore::WaveTrack*)(_arranger->curTrack());



   MusECore::SndFileR f = MusECore::getWave(name, true);

   if (f.isNull()) {
      ERROR_WAVE(stderr, "import audio file failed\n");
      return true;
   }
   track->setChannels(f->channels());
   track->resetMeter();
   int samples = f->samples();
   if (MusEGlobal::sampleRate != f->samplerate()) {
      if(QMessageBox::question(this, tr("Import Wavefile"),
                               tr("This wave file has a samplerate of %1,\n"
                                  "as opposed to current setting %2.\n"
                                  "File will be resampled from %1 to %2 Hz.\n"
                                  "Do you still want to import it?").arg(f->samplerate()).arg(MusEGlobal::sampleRate),
                               tr("&Yes"), tr("&No"),
                               QString::null, 0, 1 ))
      {
         return true; // this removed f from the stack, dropping refcount maybe to zero and maybe deleting the thing
      }

// REMOVE Tim. samplerate. Removed. TESTING Audio converters. Reinstate!
#if 0
      //save project if necessary
      //copy wave to project's folder,
      //rename it if there is a duplicate,
      //resample to project's rate

      if(MusEGlobal::museProject == MusEGlobal::museProjectInitPath)
      {
         if(!MusEGlobal::muse->saveAs())
            return true;
      }

      QFileInfo fi(f.name());
      QString projectPath = MusEGlobal::museProject + QDir::separator();
      QString fExt = "wav";
      QString fBaseName = fi.baseName();
      QString fNewPath = "";
      bool bNameIsNotUsed = false;
      for(int i = 0; i < 1000; i++)
      {
         fNewPath = projectPath + fBaseName + ((i == 0) ? "" : QString::number(i)) +  "." + fExt;
         if(!QFile(fNewPath).exists())
         {
            bNameIsNotUsed = true;
            break;
         }
      }

      if(!bNameIsNotUsed)
      {
         QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                               tr("There are too many wave files\n"
                                  "of the same base name as imported wave file\n"
                                  "Can not continue."));
         return true;
      }

      SF_INFO sfiNew;
      sfiNew.channels = f.channels();
      sfiNew.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
      sfiNew.frames = 0;
      sfiNew.samplerate = MusEGlobal::sampleRate;
      sfiNew.seekable = 1;
      sfiNew.sections = 0;

      SNDFILE *sfNew = sf_open(fNewPath.toUtf8().constData(), SFM_RDWR, &sfiNew);
      if(sfNew == NULL)
      {
         QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                               tr("Can't create new wav file in project folder!\n") + sf_strerror(NULL));
         return true;
      }

      int srErr = 0;
      SRC_STATE *srState = src_new(SRC_SINC_BEST_QUALITY, sfiNew.channels, &srErr);
      if(!srState)
      {
         QMessageBox::critical(MusEGlobal::muse, tr("Wave import error"),
                               tr("Failed to initialize sample rate converter!"));
         sf_close(sfNew);
         QFile(fNewPath).remove();
         return true;
      }



      float fPeekMax = 1.0f; //if output save file will peek above this walue
      //it should be normalized later
      float fNormRatio = 1.0f / fPeekMax;
      int nTriesMax = 5;
      int nCurTry = 0;
      do
      {
         QProgressDialog pDlg(MusEGlobal::muse);
         pDlg.setMinimum(0);
         pDlg.setMaximum(f.samples());
         pDlg.setCancelButtonText(tr("Cancel"));
         if(nCurTry == 0)
         {
            pDlg.setLabelText(tr("Resampling wave file\n"
                                    "\"%1\"\n"
                                    "from %2 to %3 Hz...")
                                 .arg(f.name()).arg(f.samplerate()).arg(sfiNew.samplerate));
         }
         else
         {
            pDlg.setLabelText(tr("Output has clipped\n"
                                 "Resampling again and normalizing wave file\n"
                                 "\"%1\"\n"
                                 "Try %2 of %3...")
                              .arg(QFileInfo(fNewPath).fileName()).arg(nCurTry).arg(nTriesMax));
         }
         pDlg.setWindowModality(Qt::WindowModal);
         src_reset(srState);
         SRC_DATA sd;
         sd.src_ratio = ((double)MusEGlobal::sampleRate) / (double)f.samplerate();
         sf_count_t szBuf = 8192;
         float srcBuffer [szBuf];
         float dstBuffer [szBuf];
         unsigned sChannels = f.channels();
         sf_count_t szBufInFrames = szBuf / sChannels;
         sf_count_t szFInFrames = f.samples();
         sf_count_t nFramesRead = 0;
         sf_count_t nFramesWrote = 0;
         sd.end_of_input = 0;
         bool bEndOfInput = false;
         pDlg.setValue(0);

         f.seek(0, SEEK_SET);

         while(sd.end_of_input == 0)
         {
            size_t nFramesBuf = 0;
            if(bEndOfInput)
               sd.end_of_input = 1;
            else
            {
               nFramesBuf = f.readDirect(srcBuffer, szBufInFrames);
               if(nFramesBuf == 0)
                  break;
               nFramesRead += nFramesBuf;
            }

            sd.data_in = srcBuffer;
            sd.data_out = dstBuffer;
            sd.input_frames = nFramesBuf;
            sd.output_frames = szBufInFrames;
            sd.input_frames_used = 0;
            sd.output_frames_gen = 0;
            do
            {
               if(src_process(srState, &sd) != 0)
                  break;
               sd.data_in += sd.input_frames_used * sChannels;
               sd.input_frames -= sd.input_frames_used;

               if(sd.output_frames_gen > 0)
               {
                  nFramesWrote += sd.output_frames_gen;
                  //detect maximum peek value;
                  for(unsigned ch = 0; ch < sChannels; ch++)
                  {

                     for(long k = 0; k < sd.output_frames_gen; k++)
                     {
                        dstBuffer [k * sChannels + ch] *= fNormRatio; //normilize if needed
                        float fCurPeek = dstBuffer [k * sChannels + ch];
                        if(fPeekMax < fCurPeek)
                        {
                           //update maximum peek value
                           fPeekMax = fCurPeek;
                        }
                     }
                  }
                  sf_writef_float(sfNew, dstBuffer, sd.output_frames_gen);
               }
               else
                  break;

            }
            while(true);

            pDlg.setValue(nFramesRead);

            if(nFramesRead >= szFInFrames)
            {
               bEndOfInput = true;
            }

            if(pDlg.wasCanceled())//free all resources
            {
               src_delete(srState);
               sf_close(sfNew);
               f.close();
               f = NULL;
               QFile(fNewPath).remove();
               return true;
            }
         }

         pDlg.setValue(szFInFrames);

         if(fPeekMax > 1.0f) //output has clipped. Normilize it
         {
            nCurTry++;
            sf_seek(sfNew, 0, SEEK_SET);
            f.seek(0, SEEK_SET);
            pDlg.setValue(0);
            fNormRatio = 1.0f / fPeekMax;
            fPeekMax = 1.0f;
         }
         else
            break;
      }
      while(nCurTry <= nTriesMax);

      src_delete(srState);

      sf_close(sfNew);

      f.close();
      f = NULL;

      //reopen resampled wave again
      f = MusECore::getWave(fNewPath, true);
      if(!f)
      {
         printf("import audio file failed\n");
         return true;
      }
      samples = f->samples();
#endif
   }

   MusECore::WavePart* part = new MusECore::WavePart((MusECore::WaveTrack *)track);
   if (tick)
      part->setTick(tick);
   else
      part->setTick(MusEGlobal::song->cpos());
   part->setLenFrame(samples);

   MusECore::Event event(MusECore::Wave);
   MusECore::SndFileR sf(f);
   event.setSndFile(sf);
   event.setSpos(0);
   event.setLenFrame(samples);
   part->addEvent(event);

   part->setName(QFileInfo(f->name()).completeBaseName());
   MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddPart, part));
   unsigned endTick = part->tick() + part->lenTick();
   if (MusEGlobal::song->len() < endTick)
      MusEGlobal::song->setLen(endTick);
   return false;
}

} // namespace MusEGui

