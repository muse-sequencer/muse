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


#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "muse_math.h"
#include <samplerate.h>

#include <QProgressDialog>

#include "wave.h"
#include "type_defs.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_WAVE(dev, format, args...) fprintf(dev, format, ##args)
#define INFO_WAVE(dev, format, args...) // fprintf(dev, format, ##args)
#define DEBUG_WAVE(dev, format, args...)  // fprintf(dev, format, ##args)

namespace MusECore {

const int cacheMag = 128;

// static
SndFileList* SndFile::_sndFiles = nullptr;
AudioConverterPluginList* SndFile::_pluginList = nullptr;
AudioConverterSettingsGroup** SndFile::_defaultSettings = nullptr;
int SndFile::_systemSampleRate = 0;
int SndFile::_segSize = 0;

// static
void SndFile::initWaveModule(
  SndFileList* sndFiles,
  AudioConverterPluginList* pluginList, 
  AudioConverterSettingsGroup** defaultSettings,
  int systemSampleRate,
  int segSize)
{
  _sndFiles = sndFiles;
  _pluginList = pluginList;
  _defaultSettings = defaultSettings;
  _systemSampleRate = systemSampleRate;
  _segSize = segSize;
}

sf_count_t sndfile_vio_get_filelen(void *user_data)
{
  return ((SndFile*)user_data)->virtualData()._virtualBytes;
}

sf_count_t sndfile_vio_seek(sf_count_t offset, int whence, void *user_data)
{
  SndFile* sf = (SndFile*)user_data;
  SndFileVirtualData& vd = sf->virtualData();
  if(!vd._virtualData)
    return -1;
  switch(whence)
  {
    case SEEK_CUR:
      if(vd._virtualCurPos + offset < 0 || vd._virtualCurPos + offset >= vd._virtualBytes)
        return -1;
      vd._virtualCurPos += offset;
    break;

    case SEEK_END:
      if(vd._virtualBytes + offset < 0 || vd._virtualBytes + offset >= vd._virtualBytes)
        return -1;
      vd._virtualCurPos = vd._virtualBytes + offset;
    break;

    default:
      if(offset < 0 || offset >= vd._virtualBytes)
        return -1;
      vd._virtualCurPos = offset;
    break;
  }

  return vd._virtualCurPos;
}

sf_count_t sndfile_vio_read(void *ptr, sf_count_t count, void *user_data)
{
  SndFile* sf = (SndFile*)user_data;
  SndFileVirtualData& vd = sf->virtualData();
  if(!vd._virtualData)
    return 0;
  if(vd._virtualCurPos >= vd._virtualBytes)
    return 0;
  if(vd._virtualCurPos + count > vd._virtualBytes)
    count = vd._virtualBytes - vd._virtualCurPos;
  // TODO Defer to AL::DSP for this?
  std::memcpy(ptr, (const char*)vd._virtualData + vd._virtualCurPos, count);
  vd._virtualCurPos += count;
  return count;
}

sf_count_t sndfile_vio_write(const void *ptr, sf_count_t count, void *user_data)
{
  SndFile* sf = (SndFile*)user_data;
  SndFileVirtualData& vd = sf->virtualData();
  if(!vd._virtualData)
    return 0;
  if(vd._virtualCurPos >= vd._virtualBytes)
    return 0;
  if(vd._virtualCurPos + count > vd._virtualBytes)
    count = vd._virtualBytes - vd._virtualCurPos;
  // TODO Defer to AL::DSP for this?
  std::memcpy((char*)vd._virtualData + vd._virtualCurPos, ptr, count);
  vd._virtualCurPos += count;
  return count;
}

sf_count_t sndfile_vio_tell(void *user_data)
{
  return ((SndFile*)user_data)->virtualData()._virtualCurPos;
}

SF_VIRTUAL_IO sndfile_vio
{
  sndfile_vio_get_filelen,
  sndfile_vio_seek,
  sndfile_vio_read,
  sndfile_vio_write,
  sndfile_vio_tell
};

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

SndFile::SndFile(
  const QString& name,
  bool installConverter,
  bool isOffline)
  : _isOffline(isOffline), _useConverter(installConverter)
      {
      _stretchList = nullptr;
      _audioConverterSettings = nullptr;
      if(_useConverter)
      {
        _stretchList = new StretchList();
        // true = Local settings, initialized to -1.
        _audioConverterSettings = new AudioConverterSettingsGroup(true); // Local settings.
        if(_pluginList)
          _audioConverterSettings->populate(_pluginList, true);
      }
      
      finfo = new QFileInfo(name);
      sf    = nullptr;
      sfUI  = nullptr;
      csize = 0;
      cache = nullptr;
      openFlag = false;
      if(_sndFiles)
        _sndFiles->push_back(this);
      refCount = 0;
      writeBuffer = nullptr;
      writeSegSize = std::max((size_t)_segSize, (size_t)cacheMag);// cache minimum segment size for write operations
      
      _staticAudioConverter    = nullptr;
      _staticAudioConverterUI  = nullptr;
      _dynamicAudioConverter   = nullptr;
      _dynamicAudioConverterUI = nullptr;
      }

SndFile::SndFile(
  void* virtualData,
  sf_count_t virtualBytes,
  bool installConverter,
  bool isOffline)
  : _isOffline(isOffline), _useConverter(installConverter),
    _virtualData(SndFileVirtualData(virtualData, virtualBytes))
{
      _stretchList = nullptr;
      _audioConverterSettings = nullptr;
      if(_useConverter)
      {
        _stretchList = new StretchList();
        // true = Local settings, initialized to -1.
        _audioConverterSettings = new AudioConverterSettingsGroup(true); // Local settings.
        if(_pluginList)
          _audioConverterSettings->populate(_pluginList, true);
      }
      
      finfo = nullptr;
      sf    = nullptr;
      sfUI  = nullptr;
      csize = 0;
      cache = nullptr;
      openFlag = false;
      //if(_sndFiles)
      //  _sndFiles->push_back(this);
      refCount = 0;
      writeBuffer = nullptr;
      writeSegSize = std::max((size_t)_segSize, (size_t)cacheMag);// cache minimum segment size for write operations
      
      _staticAudioConverter    = nullptr;
      _staticAudioConverterUI  = nullptr;
      _dynamicAudioConverter   = nullptr;
      _dynamicAudioConverterUI = nullptr;
}

SndFile::~SndFile()
      {
      DEBUG_WAVE(stderr, "SndFile dtor this:%p\n", this);
      if (openFlag)
            close();
      if(_sndFiles)
      {
        for (iSndFile i = _sndFiles->begin(); i != _sndFiles->end(); ++i) {
            if (*i == this) {
                  //DEBUG_WAVE(stderr, "erasing from sndfiles:%s\n", finfo->canonicalFilePath().toLatin1().constData());
                  _sndFiles->erase(i);
                  break;
                  }
            }
      }
      if(finfo)
        delete finfo;
      if (cache)
        delete[] cache;
      if(writeBuffer)
         delete [] writeBuffer;

      if(_stretchList)
        delete _stretchList;
      if(_audioConverterSettings)
        delete _audioConverterSettings;
      }

int SndFile::getRefCount() const   { return refCount; }
bool SndFile::isOpen() const       { return openFlag; }
bool SndFile::isWritable() const   { return writeFlag; }
bool SndFile::useConverter() const { return _useConverter; }
AudioConverterSettingsGroup* SndFile::audioConverterSettings() const { return _audioConverterSettings; }
StretchList* SndFile::stretchList() const { return _stretchList; }

bool SndFile::isOffline()
{
  if(_staticAudioConverter)
    _isOffline = _staticAudioConverter->mode() == AudioConverterSettings::OfflineMode;
  return _isOffline;
}

bool SndFile::setOffline(
  bool v)
{
  // Check if the current mode is already in the requested mode.
  if(isOffline() == v)
    return false;

  _isOffline = v;

  // Delete the current converter, if any.
  AudioConverterPluginI* converter = staticAudioConverter(AudioConverterSettings::RealtimeMode);
  if(converter)
    delete converter;
  converter = nullptr;
  
  if(useConverter() && audioConverterSettings())
  {
    const AudioConverterSettingsGroup* settings = audioConverterSettings()->useSettings() ?
      audioConverterSettings() : *_defaultSettings;
    const bool isLocalSettings = audioConverterSettings()->useSettings();

    const bool doStretch = isStretched();
    const bool doResample = isResampled();

    // For offline mode, we COULD create a third converter just for it, apart from the main
    //  and UI converters. But our system doesn't have a third converter (yet) - and it may
    //  or may not get one, we'll see. Still, the operation supports setting it, in case.
    // So instead, in offline mode we switch out the main converter for one with with offline settings.
    converter = setupAudioConverter(
      settings,
      *_defaultSettings,
      isLocalSettings,
      v ? AudioConverterSettings::OfflineMode : AudioConverterSettings::RealtimeMode,
      doResample,
      doStretch);
  }
  setStaticAudioConverter(converter, AudioConverterSettings::RealtimeMode);
  return true;
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

      // File based:
      if(finfo)
      {
        QString p = path();
        if(p.isEmpty())
          return true;
        sfinfo.format = 0;
        sfUI = nullptr;
        sf = sf_open(p.toLocal8Bit().constData(), SFM_READ, &sfinfo);
        if (!sf)
              return true;
        if(finfo && createCache){
          sfinfo.format = 0;
          sfUI = sf_open(p.toLocal8Bit().constData(), SFM_READ, &sfinfo);
          if (!sfUI){
              sf_close(sf);
              sf = nullptr;
              return true;
          }
        }
      }
      // Memory based:
      else
      {
        if(!_virtualData._virtualData)
          return true;
        //sfinfo.format = 0; // Supplied by caller via setFormat.
        sfUI = nullptr;
        sf = sf_open_virtual(&sndfile_vio, SFM_READ, &sfinfo, this);
        if (!sf)
          return true;
      }

      if(useConverter())
      {
        _staticAudioConverter = setupAudioConverter(
          audioConverterSettings(),
          *_defaultSettings,
          true,  // true = Local settings.
          isOffline() ?
            AudioConverterSettings::OfflineMode :
            AudioConverterSettings::RealtimeMode,
          isResampled(),
          isStretched());

        if(finfo)
          _staticAudioConverterUI = setupAudioConverter(
            audioConverterSettings(), 
            *_defaultSettings,
            true,  // true = Local settings.
            AudioConverterSettings::GuiMode, 
            isResampled(),
            isStretched());
      }
      
      writeFlag = false;
      openFlag  = true;

      if (finfo && createCache) {
        QString cacheName = finfo->absolutePath() + QString("/") + finfo->completeBaseName() + QString(".wca");
        readCache(cacheName, showProgress);
      }
      return false;
      }

AudioConverterPluginI* SndFile::setupAudioConverter(
  const AudioConverterSettingsGroup* settings, 
  const AudioConverterSettingsGroup* defaultSettings,
  bool isLocalSettings, 
  AudioConverterSettings::ModeType mode, 
  bool doResample,
  bool doStretch) const
{
  if(!useConverter() || !defaultSettings || !_pluginList)
    return nullptr;
  
  AudioConverterPluginI* plugI = nullptr;
  
  int pref_resampler = 
    (settings && (settings->_options._useSettings || !isLocalSettings)) ? 
      settings->_options._preferredResampler :
      defaultSettings->_options._preferredResampler;
  
  int pref_shifter = 
    (settings && (settings->_options._useSettings || !isLocalSettings)) ? 
      settings->_options._preferredShifter:
      defaultSettings->_options._preferredShifter;

  AudioConverterSettingsI* def_res_settings = nullptr;
  AudioConverterSettingsI* local_res_settings = nullptr;
  AudioConverterSettingsI* rt_res_settings = nullptr;
  AudioConverterPlugin* res_plugin = _pluginList->find(0, pref_resampler, AudioConverter::SampleRate);
  if(res_plugin)
  {
    if(isLocalSettings)
    {
      def_res_settings = defaultSettings->find(
          defaultSettings->_options._preferredResampler);
      local_res_settings = settings ? settings->find(pref_resampler) : nullptr;
      rt_res_settings = (local_res_settings && local_res_settings->useSettings(mode)) ? local_res_settings : def_res_settings;
    }
    else
      rt_res_settings = settings->find(pref_resampler);
  }

  AudioConverterSettingsI* def_str_settings = nullptr;
  AudioConverterSettingsI* local_str_settings = nullptr;
  AudioConverterSettingsI* rt_str_settings = nullptr;
  AudioConverterPlugin* str_plugin = _pluginList->find(0, pref_shifter, AudioConverter::Stretch);
  if(str_plugin)
  {
    if(isLocalSettings)
    {
      def_str_settings = defaultSettings->find(
          defaultSettings->_options._preferredShifter);
      local_str_settings = settings ? settings->find(pref_shifter) : nullptr;
      rt_str_settings = (local_str_settings && local_str_settings->useSettings(mode)) ? local_str_settings : def_str_settings;
      
      if(rt_str_settings) { } // Dummy for now, to avoid unused warning.
    }
    else
      rt_str_settings = settings->find(pref_shifter);
  }

  if(sf && (sampleRateDiffers() || doResample || doStretch))
  {
    // Realtime audio section...
    // Create a new converter.
    
    AudioConverterPlugin* fin_plug  = 
      doStretch ? str_plugin : (res_plugin ? res_plugin : str_plugin);
      
    AudioConverterSettingsI* fin_set = 
      doStretch ? rt_str_settings : (rt_res_settings ? rt_res_settings : rt_str_settings);
      
    if(fin_set && fin_plug && (fin_plug->maxChannels() < 0 || sfinfo.channels <= fin_plug->maxChannels()))
    {
      plugI = new AudioConverterPluginI();
      plugI->initPluginInstance(
        fin_plug,
        _systemSampleRate,
        sfinfo.channels,
        fin_set->settings(), 
        mode);
    }
  }
    
return plugI;
}

AudioConverterPluginI* SndFile::staticAudioConverter(AudioConverterSettings::ModeType mode) const 
{ 
  switch(mode)
  {
    case AudioConverterSettings::OfflineMode:
      return nullptr;  // TODO ?
    break;

    case AudioConverterSettings::RealtimeMode:
      return _staticAudioConverter;
    break;

    case AudioConverterSettings::GuiMode:
      return _staticAudioConverterUI;
    break;
  }
  return nullptr; 
}

void SndFile::setStaticAudioConverter(AudioConverterPluginI* converter, AudioConverterSettings::ModeType mode)
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
  if(_staticAudioConverter && _staticAudioConverter->maxStretchRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverter->maxStretchRatio() < m))
    m = _staticAudioConverter->maxStretchRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxStretchRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverterUI->maxStretchRatio() < m))
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
  if(_staticAudioConverter && _staticAudioConverter->maxSamplerateRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverter->maxSamplerateRatio() < m))
    m = _staticAudioConverter->maxSamplerateRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxSamplerateRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverterUI->maxSamplerateRatio() < m))
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
  if(_staticAudioConverter && _staticAudioConverter->maxPitchShiftRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverter->maxPitchShiftRatio() < m))
    m = _staticAudioConverter->maxPitchShiftRatio();
  if(_staticAudioConverterUI && _staticAudioConverterUI->maxPitchShiftRatio() > 0.0 &&
     (m < 0.0 || _staticAudioConverterUI->maxPitchShiftRatio() < m))
    m = _staticAudioConverterUI->maxPitchShiftRatio();
  return m;
}

void SndFile::setAudioConverterSettings(AudioConverterSettingsGroup* settings)
{ 
  _audioConverterSettings = settings;
}

//---------------------------------------------------------
//   update
//    called after recording to file
//---------------------------------------------------------

void SndFile::update(bool showProgress)
      {
      if(!finfo)
        return;

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
   if(!finfo || cstart >= csize)
      return;
   QProgressDialog* progress = nullptr;
   if (showProgress) {
      QString label(QWidget::tr("Create peakfile for "));
      label += basename();
      progress = new QProgressDialog(label,
                                     QString(), 0, csize, 0);
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

//---------------------------------------------------------
//   readCache
//---------------------------------------------------------

void SndFile::readCache(const QString& path, bool showProgress)
{
   if(!finfo)
     return;

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
//   writeCache
//---------------------------------------------------------

void SndFile::writeCache(const QString& path)
      {
      if(!finfo)
        return;

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
      if(!finfo)
        return;

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

//---------------------------------------------------------
//   readConverted
//   The offset is the offset into the sound file and is NOT converted.
//---------------------------------------------------------

void SndFile::readConverted(SampleV* s, int mag, sf_count_t pos, sf_count_t offset, bool overwrite, bool allowSeek)
      {
      if(!finfo)
        return;

      if(!useConverter() ||
         !(_staticAudioConverterUI && _staticAudioConverterUI->isValid() &&
          (((sampleRateDiffers() || isResampled()) && (_staticAudioConverterUI->capabilities() & AudioConverter::SampleRate)) ||
           (isStretched() && (_staticAudioConverterUI->capabilities() & AudioConverter::Stretch))) ))
      {
        read(s, mag, offset + pos, overwrite, allowSeek);
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
// Removed. NOTE Seeking is now done only once in the graphic code.
//       if(allowSeek && convertPosition(pos) > convertPosition(samples()))
//             return;

      if (mag < cacheMag) {
            float data[srcChannels][mag];
            float* fp[srcChannels];
            for (int i = 0; i < srcChannels; ++i)
                  fp[i] = &data[i][0];

            sf_count_t ret = 0;
            const sf_count_t n        = mag;

            if(sfUI)
              ret = _staticAudioConverterUI->process(
                sfUI, channels(), sampleRateRatio(), stretchList(), pos, fp, srcChannels, n, true);
            else
              ret = _staticAudioConverter->process(
                sf, channels(), sampleRateRatio(), stretchList(), pos, fp, srcChannels, n, true);
            
            if(ret != n)
              return;

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
            sf_count_t rest = csize - ((offset + convertPosition(pos))/cacheMag);
            sf_count_t end  = mag;
            if (rest < mag)
                  end = rest;

            for (int ch = 0; ch < srcChannels; ++ch) {
                  int rms = 0;
                  sf_count_t off = (offset + convertPosition(pos))/cacheMag;
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

      // File based:
      if(finfo)
      {
        const QString p = path();
        if(p.isEmpty())
          return true;
        sf = sf_open(p.toLocal8Bit().constData(), SFM_RDWR, &sfinfo);
      }
      // Memory based:
      else
      {
        if(!_virtualData._virtualData)
          return true;
        sf = sf_open_virtual(&sndfile_vio, SFM_RDWR, &sfinfo, this);
      }

      sfUI = nullptr;
      if (sf) {
            if(writeBuffer)
              delete [] writeBuffer;
            writeBuffer = new float [writeSegSize * std::max(2, sfinfo.channels)];
            openFlag  = true;
            writeFlag = true;
            if(finfo)
            {
              QString cacheName = finfo->absolutePath() +
                QString("/") + finfo->completeBaseName() + QString(".wca");
              readCache(cacheName, true);
            }
          }
      return !sf;
      }

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SndFile::close()
      {
      DEBUG_WAVE(stderr, "SndFile::close this:%p\n", this);
      if (!openFlag) {
            DEBUG_WAVE(stderr, "SndFile:: alread closed\n");
            return;
            }
      if(int err = sf_close(sf))
      {
        err += 0; // Touch.
        ERROR_WAVE(stderr, "SndFile::close Error:%d on sf_close(sf:%p)\n", err, sf);
      }
      else
        sf = nullptr;
      if (sfUI)
      {
            if(int err = sf_close(sfUI))
            {
              err += 0; // Touch.
              ERROR_WAVE(stderr, "SndFile::close Error:%d on sf_close(sfUI:%p)\n", err, sfUI);
            }
            else
              sfUI = nullptr;
      }
      openFlag = false;
      
      if(_staticAudioConverter)
      {
        delete _staticAudioConverter;
        _staticAudioConverter = nullptr;
      }
      if(_staticAudioConverterUI)
      {
        delete _staticAudioConverterUI;
        _staticAudioConverterUI = nullptr;
      }
      if(_dynamicAudioConverter)
      {
        delete _dynamicAudioConverter;
        _dynamicAudioConverter = nullptr;
      }
      if(_dynamicAudioConverterUI)
      {
        delete _dynamicAudioConverterUI;
        _dynamicAudioConverterUI = nullptr;
      }

      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void SndFile::remove()
      {
      if (openFlag)
            close();
      if(finfo)
        QFile::remove(finfo->filePath());
      }

QString SndFile::basename() const
      {
      return finfo ? finfo->completeBaseName() : QString();
      }

QString SndFile::path() const
      {
      return finfo ? finfo->filePath() : QString();
      }

QString SndFile::canonicalPath() const
      {
      return finfo ? finfo->canonicalFilePath() : QString();
      }

QString SndFile::dirPath() const
      {
      return finfo ? finfo->absolutePath() : QString();
      }

QString SndFile::canonicalDirPath() const
      {
      return finfo ? finfo->canonicalPath() : QString();
      }

QString SndFile::name() const
      {
      return finfo ? finfo->fileName() : QString();
      }

      
//---------------------------------------------------------
//   sampleRateRatio
//---------------------------------------------------------

double SndFile::sampleRateRatio() const
{
  return (double)sfinfo.samplerate / (double)_systemSampleRate;
}

//---------------------------------------------------------
//   sampleRateDiffers
//---------------------------------------------------------

bool SndFile::sampleRateDiffers() const
{
  return sfinfo.samplerate != _systemSampleRate;
}

bool SndFile::isStretched() const 
{ 
  if(!_stretchList)
    return false;
  return _stretchList->isStretched();
}

bool SndFile::isPitchShifted() const
{ 
  if(!_stretchList)
    return false;
  return _stretchList->isPitchShifted();
}

bool SndFile::isResampled() const 
{ 
  if(!_stretchList)
    return false;
  return _stretchList->isResampled();
}

//---------------------------------------------------------
//   convertPosition
//---------------------------------------------------------

sf_count_t SndFile::convertPosition(sf_count_t pos) const
{
  double new_pos = pos;
  if(useConverter() && _staticAudioConverter && _stretchList)
  {
    int type = 0;
    if(_staticAudioConverter->capabilities() & AudioConverter::Stretch)
      type |= StretchListItem::StretchEvent;
    if(_staticAudioConverter->capabilities() & AudioConverter::SampleRate)
      type |= StretchListItem::SamplerateEvent;

    if(_staticAudioConverter->capabilities() & AudioConverter::SampleRate)
      new_pos *= sampleRateRatio();
    
    if(type != 0)
      new_pos = _stretchList->unSquish(new_pos, type);
  }

  return new_pos;
}

//---------------------------------------------------------
//   samples
//---------------------------------------------------------

sf_count_t SndFile::samples() const
      {
      if (!finfo || !writeFlag) // if file is read only sfinfo is reliable
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
//   samplesConverted
//---------------------------------------------------------

sf_count_t SndFile::samplesConverted() const
      {
      if(sfinfo.samplerate == 0 || _systemSampleRate == 0)
        return 0;
      return samples() / sampleRateRatio();
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

void SndFile::setFormat(int fmt, int ch, int rate, sf_count_t frames)
      {
      sfinfo.samplerate = rate;
      sfinfo.channels   = ch;
      sfinfo.format     = fmt;
      sfinfo.seekable   = true;
      sfinfo.frames     = frames;
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

sf_count_t SndFile::readConverted(sf_count_t pos, int srcChannels,
                                  float** buffer, sf_count_t frames, bool overwrite)
{
  if(useConverter() && _staticAudioConverter && _staticAudioConverter->isValid() &&
     (((sampleRateDiffers() || isResampled()) && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate)) ||
      (isStretched() && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))) )
  {
    return _staticAudioConverter->process(
      sf, channels(), sampleRateRatio(), stretchList(), pos, buffer, srcChannels, frames, overwrite);
  }
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

size_t SndFile::write(int srcChannels, float** src, size_t n, bool liveWaveUpdate)
{
   size_t wrFrames = 0;

   if(n <= writeSegSize)
       wrFrames = realWrite(srcChannels, src, n, wrFrames, liveWaveUpdate);
   else
   {
      while(1)
      {
         size_t sz = (n - wrFrames) < writeSegSize ? (n - wrFrames) : writeSegSize;
         size_t nrWrote = realWrite(srcChannels, src, sz, wrFrames, liveWaveUpdate);
         if(nrWrote == 0) // Nothing written?
           break;
         wrFrames += nrWrote;
         if(wrFrames >= n)
           break;
      }
   }
   return wrFrames;
}

size_t SndFile::realWrite(int srcChannels, float** src, size_t n, size_t offs, bool liveWaveUpdate)
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

   if(liveWaveUpdate)
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

sf_count_t SndFile::seek(sf_count_t frames, int whence)
      {
      return sf_seek(sf, frames, whence);
      }

sf_count_t SndFile::seekUI(sf_count_t frames, int whence)
{
  sf_count_t rn = 0;
  if(sfUI)
    rn = sf_seek(sfUI, frames, whence);
  else if(sf)
    rn = sf_seek(sf, frames, whence);
  return rn;
}

//---------------------------------------------------------
//   seekUIConverted
//   The offset is the offset into the sound file and is NOT converted.
//---------------------------------------------------------

sf_count_t SndFile::seekUIConverted(sf_count_t frames, int whence, sf_count_t offset)
{
  const sf_count_t smps = samples();
  sf_count_t rn = 0;
  sf_count_t pos = offset + convertPosition(frames);
  if(pos < 0)
    pos = 0;
  // Clamp it at 'one past the end' in other words EOF.
  if(pos > smps)
    pos = smps;
  
  if(sfUI)
  {
    rn = sf_seek(sfUI, pos, whence);
    // Reset the converter. Its current state is meaningless now.
    if(useConverter() && _staticAudioConverterUI)
      _staticAudioConverterUI->reset();
  }
  else if(sf)
  {
    rn = sf_seek(sf, pos, whence);
    // Reset the converter. Its current state is meaningless now.
    if(useConverter() && _staticAudioConverter)
      _staticAudioConverter->reset();
  }
  return rn;
}

//---------------------------------------------------------
//   seekConverted
//   The offset is the offset into the sound file and is NOT converted.
//---------------------------------------------------------

sf_count_t SndFile::seekConverted(sf_count_t frames, int whence, int offset)
      {
      if(useConverter() && _staticAudioConverter && _staticAudioConverter->isValid() &&
         (((sampleRateDiffers() || isResampled()) && (_staticAudioConverter->capabilities() & AudioConverter::SampleRate)) ||
          (isStretched() && (_staticAudioConverter->capabilities() & AudioConverter::Stretch))) )
      {
        const sf_count_t smps = samples();

        // Do not convert the offset.
        sf_count_t pos = offset + convertPosition(frames);
        if(pos < 0)
          pos = 0;
        // Clamp it at 'one past the end' in other words EOF.
        if(pos > smps)
          pos = smps;

        const sf_count_t rn = sf_seek(sf, pos, whence);
        
        // Reset the converter. Its current state is meaningless now.
        _staticAudioConverter->reset();
      
        return rn;
      }
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
      return nullptr;
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
      *this=nullptr; // decrease the refcounter, maybe delete
      }

} // namespace MusECore

