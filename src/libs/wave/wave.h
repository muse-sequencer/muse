//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wave.h,v 1.5.2.7 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999/2004 Werner Schweer (ws@seh.de)
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

#ifndef __WAVE_H__
#define __WAVE_H__

#include <list>
#include <vector>
#include <sndfile.h>
#include <atomic>
// For atomic<U*>, according to docs.
#include <memory>

#include <QString>
#include <QFileInfo>

#include "muse_time.h"
#include "time_stretch.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"

namespace MusECore {

//---------------------------------------------------------
//   SndFileVirtualData
//    For virtual (memory or stream) operation
//---------------------------------------------------------

class SndFileVirtualData
{
  public:
    void* _virtualData;
    sf_count_t _virtualBytes;
    sf_count_t _virtualCurPos;

    SndFileVirtualData() : _virtualData(nullptr), _virtualBytes(0), _virtualCurPos(0) { }
    SndFileVirtualData(void* virtualData, const sf_count_t virtualBytes)
      : _virtualData(virtualData), _virtualBytes(virtualBytes), _virtualCurPos(0) { }
};

//---------------------------------------------------------
//   SampleV
//    peak file value
//---------------------------------------------------------

struct SampleV {
      unsigned char peak;
      unsigned char rms;
      };

typedef std::vector<SampleV> SampleVtype;

class SndFileList;

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

class SndFile {
      QFileInfo* finfo;
      SNDFILE* sf;
      SNDFILE* sfUI;
      AudioConverterPluginI* _staticAudioConverter;
      AudioConverterPluginI* _staticAudioConverterUI;
      AudioConverterPluginI* _dynamicAudioConverter;
      AudioConverterPluginI* _dynamicAudioConverterUI;
      AudioConverterSettingsGroup* _audioConverterSettings;
      StretchList* _stretchList;
      // Whether the converter is in offline mode.
      // Always use isOffline() to check instead of reading this directly.
      bool _isOffline;
      // Whether to use any converter(s) at all.
      bool _useConverter;

      SF_INFO sfinfo;
      SampleVtype* cache;
      sf_count_t csize;                    //!< frames in cache

      // For virtual (memory or stream) operation:
      SndFileVirtualData _virtualData;

      float *writeBuffer;
      size_t writeSegSize;

      void writeCache(const QString& path);

      bool openFlag;
      bool writeFlag;
      size_t readInternal(int srcChannels, float** dst, size_t n, bool overwrite, float *buffer);
      size_t realWrite(int srcChannels, float** src, size_t n, size_t offs = 0, bool liveWaveUpdate = false);
      
   protected:
      std::atomic_int refCount;

   public:
      // Constructor for file operation.
      SndFile(
        const QString& name,
        bool installConverter = true,
        bool isOffline = false);
      // Constructor for virtual (memory or stream) operation.
      // When using the virtual interface, be sure to call setFormat before opening.
      SndFile(
        void* virtualData,
        sf_count_t virtualBytes,
        bool installConverter = true,
        bool isOffline = false);

      ~SndFile();

      static AudioConverterPluginList* _pluginList;
      static AudioConverterSettingsGroup** _defaultSettings;
      static int _systemSampleRate;
      static int _segSize;
      static SndFileList* _sndFiles;

      static void initWaveModule(
        SndFileList* sndFiles,
        AudioConverterPluginList* pluginList, 
        AudioConverterSettingsGroup** defaultSettings,
        int systemSampleRate,
        int segSize);

      int getRefCount() const;

      // Whether to use any converter(s) at all.
      bool useConverter() const;

      // Whether the converter is in offline mode.
      // Always use isOffline() to check instead of reading the member directly.
      bool isOffline();
      // Set the converter offline mode.
      // Returns whether the mode was actually changed.
      bool setOffline(bool v);

      // For virtual (memory or stream) operation.
      SndFileVirtualData& virtualData() { return _virtualData; }

      void createCache(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart = 0);
      void readCache(const QString& path, bool progress);

      // Creates a new converter based on the supplied settings and AudioConverterSettings::ModeType mode.
      // If isLocalSettings is true, settings is treated as a local settings which may override the 
      //  global default settings.
      // If isLocalSettings is false, settings is treated as the global default settings and is 
      //  directly used instead of the comparison to, and possible use of, the global default above.
      AudioConverterPluginI* setupAudioConverter(const AudioConverterSettingsGroup* settings,
                                                 const AudioConverterSettingsGroup* defaultSettings,
                                                 bool isLocalSettings, 
                                                 AudioConverterSettings::ModeType mode, 
                                                 bool doResample,
                                                 bool doStretch
                                                ) const;

      // When using the virtual interface, be sure to call setFormat before opening.
      //!< returns true on error
      bool openRead(bool createCache=true, bool showProgress=true);
      //!< returns true on error
      bool openWrite();
      void close();
      void remove();

      bool isOpen() const;
      // Whether the file was opened with write mode.
      bool isWritable() const;

      void update(bool showProgress = true);

      QString basename() const;     //!< filename without extension
      QString dirPath() const;      //!< path
      QString canonicalDirPath() const; //!< path, resolved (no symlinks or . .. etc)
      QString path() const;         //!< path with filename
      QString canonicalPath() const; //!< path with filename, resolved (no symlinks or . .. etc)
      QString name() const;         //!< filename
      // Whether the file itself is writable (ie also in a writable directory etc.)
      bool isFileWritable() const;

      // Ratio of the file's sample rate to the current audio sample rate.
      double sampleRateRatio() const;
      // Whether the sample rate ratio is exactly 1.
      bool sampleRateDiffers() const;
// REMOVE Tim. wave. Changed.
//       // Convert a frame position to its resampled or stretched position.
//       sf_count_t convertPosition(sf_count_t pos) const;
//       // Convert a resampled or stretched frame position to its unresampled or unstretched position.
//       sf_count_t unConvertPosition(sf_count_t pos) const;
      // Convert a frame position to its resampled or stretched position.
      double convertPosition(double pos) const;
      // Convert a resampled or stretched frame position to its unresampled or unstretched position.
      double unConvertPosition(double pos) const;
      // Returns whether ANY stretch event has a stretch ratio other than 1.0
      //  ie. the map is stretched, a stretcher must be engaged.
      bool isStretched() const;
      // Returns whether ANY stretch event has a pitch ratio other than 1.0 
      //  ie. the map is pitch shifted, a shifter must be engaged.
      bool isPitchShifted() const;
      // Returns whether ANY stretch event has a samplerate ratio other than 1.0 
      //  ie. the map is stretched, a samplerate converter must be engaged.
      bool isResampled() const;
      
      sf_count_t samples() const;
      // Returns number of samples, adjusted for file-to-system samplerate ratio.
// REMOVE Tim. wave. Changed.
//       sf_count_t samplesConverted() const;
      double samplesConverted() const;

      int channels() const;
      int samplerate() const;
      int format() const;
      void setFormat(int fmt, int ch, int rate, sf_count_t frames = 0);

      size_t read(int channel, float**, size_t, bool overwrite = true);
      size_t readWithHeap(int channel, float**, size_t, bool overwrite = true);
      size_t readDirect(float* buf, size_t n)    { return sf_readf_float(sf, buf, n); }
      size_t write(int channel, float**, size_t, bool liveWaveUpdate /*= false*/);
      size_t writeDirect(float *buf, size_t n) { return sf_writef_float(sf, buf, n); }

      // For now I must provide separate routines here, don't want to upset anything else.
      // Reads realtime audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      sf_count_t readConverted(sf_count_t pos, int srcChannels,
                               float** buffer, sf_count_t frames, bool overwrite = true);
      // Reads graphical audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      void readConverted(SampleV* s, int mag, sf_count_t pos, sf_count_t offset,
                         bool overwrite = true, bool allowSeek = true);
      // Seeks to a converted position if a samplerate or shift/stretch converter is active. Otherwise a normal seek.
      // The offset is the offset into the sound file and is NOT converted.
      sf_count_t seekConverted(sf_count_t frames, int whence, int offset);
      AudioConverterPluginI* staticAudioConverter(AudioConverterSettings::ModeType mode) const;
      void setStaticAudioConverter(AudioConverterPluginI* converter, AudioConverterSettings::ModeType mode);
      AudioConverterSettingsGroup* audioConverterSettings() const;
      void setAudioConverterSettings(AudioConverterSettingsGroup* settings);
      StretchList* stretchList() const;
      
      sf_count_t seek(sf_count_t frames, int whence);
      sf_count_t seekUI(sf_count_t frames, int whence);
      sf_count_t seekUIConverted(sf_count_t frames, int whence, sf_count_t offset);
      void read(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true);
      QString strerror() const;

      // Absolute minimum and maximum ratios depending on chosen converters. -1 means infinite, don't care.
      double minStretchRatio() const;
      double maxStretchRatio() const;
      double minSamplerateRatio() const;
      double maxSamplerateRatio() const;
      double minPitchShiftRatio() const;
      double maxPitchShiftRatio() const;

      friend class SndFileR;
      };

//---------------------------------------------------------
//   SndFileR
//    SndFile with reference count
//---------------------------------------------------------

class atomicSndFile : public std::atomic<SndFile*>
{
  public:
    inline SndFile* operator->() { return load(); }
    inline const SndFile* operator->() const { return load(); }
    inline SndFile* operator*() { return load(); }
    inline const SndFile* operator*() const { return load(); }
    inline atomicSndFile& operator=(SndFile* ptr) { store(ptr); return *this; }
    inline bool operator==(SndFile* c) const { return load() == c; }
    inline operator bool() const { return load() != nullptr; }
    inline bool isNull() const { return load() == nullptr; }

};

class SndFileR {
      atomicSndFile sf;

   public:
      SndFileR();
      SndFileR(SndFile* _sf);
      SndFileR(const SndFileR& ed);
      SndFileR& operator=(SndFile* ptr);
      SndFileR& operator=(const SndFileR& ed);
      bool operator==(const SndFileR& c) const;
      bool operator==(SndFile* c) const;
      SndFile* operator->();
      const SndFile* operator->() const;

      SndFile* operator*();
      const SndFile* operator*() const;

      operator bool() const;
      ~SndFileR();

      int getRefCount() const;

      bool isNull() const;

      // Whether to use any converter(s) at all.
      bool useConverter() const;

      // Whether the converter is in offline mode.
      // Always use isOffline() to check instead of reading the member directly.
      bool isOffline();
      // Set the converter offline mode.
      // Returns whether the mode was actually changed.
      bool setOffline(bool v);

      // Creates a new converter based on the supplied settings and AudioConverterSettings::ModeType mode.
      // If isLocalSettings is true, settings is treated as a local settings which may override the
      //  global default settings.
      // If isLocalSettings is false, settings is treated as the global default settings and is
      //  directly used instead of the comparison to, and possible use of, the global default above.
      AudioConverterPluginI* setupAudioConverter(
        const AudioConverterSettingsGroup* settings,
        const AudioConverterSettingsGroup* defaultSettings,
        bool isLocalSettings,
        AudioConverterSettings::ModeType mode,
        bool doResample,
        bool doStretch) const;

      // When using the virtual interface, be sure to call setFormat before opening.
      bool openRead(bool createCache=true);
      bool openWrite();
      void close();
      void remove();

      bool isOpen() const;
      // Whether the file was opened with write mode.
      bool isWritable() const;
      void update(bool showProgress = true);

      QString basename() const;
      QString dirPath() const;
      QString canonicalDirPath() const;
      QString path() const;
      QString canonicalPath() const;
      QString name() const;
      // Whether the file itself is writable (ie also in a writable directory etc.)
      bool isFileWritable() const;

      // Ratio of the file's sample rate to the current audio sample rate.
      inline double sampleRateRatio() const { return sf ? sf->sampleRateRatio() : 1.0; };
      // Whether the sample rate ratio is exactly 1.
      inline bool sampleRateDiffers() const { return sf ? sf->sampleRateDiffers() : false; };
// REMOVE Tim. wave. Changed.
//       // Convert a frame position to its resampled or stretched position.
//       inline sf_count_t convertPosition(sf_count_t pos) const { return sf ? sf->convertPosition(pos) : pos; };
//       // Convert a resampled or stretched frame position to its unresampled or unstretched position.
//       inline sf_count_t unConvertPosition(sf_count_t pos) const { return sf ? sf->unConvertPosition(pos) : pos; };
      // Convert a frame position to its resampled or stretched position.
      inline double convertPosition(double pos) const { return sf ? sf->convertPosition(pos) : pos; };
      // Convert a resampled or stretched frame position to its unresampled or unstretched position.
      inline double unConvertPosition(double pos) const { return sf ? sf->unConvertPosition(pos) : pos; };
      // Returns whether ANY stretch event has a stretch ratio other than 1.0
      //  ie. the map is stretched, a stretcher must be engaged.
      bool isStretched() const;
      // Returns whether ANY stretch event has a pitch ratio other than 1.0
      //  ie. the map is pitch shifted, a shifter must be engaged.
      bool isPitchShifted() const;
      // Returns whether ANY stretch event has a samplerate ratio other than 1.0
      //  ie. the map is stretched, a samplerate converter must be engaged.
      bool isResampled() const;

      sf_count_t samples() const;
      // Returns number of samples, adjusted for file-to-system samplerate ratio.
// REMOVE Tim. wave. Changed.
//       sf_count_t samplesConverted() const;
      double samplesConverted() const;

      int channels() const;
      int samplerate() const;
      int format() const;
      void setFormat(int fmt, int ch, int rate, sf_count_t frames = 0);
      size_t readWithHeap(int channel, float** f, size_t n, bool overwrite = true);
      size_t read(int channel, float** f, size_t n, bool overwrite = true);
      size_t readDirect(float* f, size_t n);

      size_t write(int channel, float** f, size_t n, bool liveWaveUpdate /*= false*/);

      // For now I must provide separate routines here, don't want to upset anything else.
      // Reads realtime audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      sf_count_t readConverted(sf_count_t pos, int channel,
                               float** buffer, sf_count_t frames, bool overwrite = true);
      // Reads graphical audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      void readConverted(SampleV* s, int mag, unsigned pos, sf_count_t offset,
                         bool overwrite = true, bool allowSeek = true);
      // Seeks to a converted position if a samplerate or shift/stretch converter is active. Otherwise a normal seek.
      // The offset is the offset into the sound file and is NOT converted.
      sf_count_t seekConverted(sf_count_t frames, int whence, int offset);
      AudioConverterPluginI* staticAudioConverter(AudioConverterSettings::ModeType mode) const;
      void setStaticAudioConverter(AudioConverterPluginI* converter, AudioConverterSettings::ModeType mode);
      AudioConverterSettingsGroup* audioConverterSettings() const;
      void setAudioConverterSettings(AudioConverterSettingsGroup* settings);
      StretchList* stretchList() const;

      // Absolute minimum and maximum ratios depending on chosen converters. -1 means infinite, don't care.
      double minStretchRatio() const;
      double maxStretchRatio() const;
      double minSamplerateRatio() const;
      double maxSamplerateRatio() const;
      double minPitchShiftRatio() const;
      double maxPitchShiftRatio() const;

      sf_count_t seek(sf_count_t frames, int whence);
      sf_count_t seekUI(sf_count_t frames, int whence);
      sf_count_t seekUIConverted(sf_count_t frames, int whence, sf_count_t offset);
      void read(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true);
      QString strerror() const;
      };

//---------------------------------------------------------
//   SndFileList
//---------------------------------------------------------

class SndFileList : public std::list<SndFile*> {
   public:
      SndFile* search(const QString& name);
      // void clearDelete(); // clearDelete MUST NOT exist! deleting is handled by the refcounting SndFileRs!
                             // this SndFileList is just for information, consider it as "weak pointers"
      };

typedef SndFileList::iterator iSndFile;
typedef SndFileList::const_iterator ciSndFile;

#if 0

class Clip;
//---------------------------------------------------------
//   ClipBase
//---------------------------------------------------------

class ClipBase {
   protected:
      QString _name;
      SndFileR f;
      int _spos;        // start sample position in WaveFile
      int len;          // len of clip
      int lrefs;        // logical references
      bool deleted;
      int refCount;

   public:
      ClipBase(const SndFileR& f, int start, int len);
      ~ClipBase();
      const QString& name() const      { return _name;  }
      void setName(const QString& s)   { _name = s;     }
      int spos() const                 { return _spos;  }
      void setSpos(int s)              { _spos = s;     }
      SndFileR file1() const           { return f;      }

      void read(unsigned, float**, int, unsigned);
      void write(int, Xml&) const;
      int samples() const              { return len; }
      void setSamples(int s)           { len = s;    }
      int getRefCount() const          { return refCount; }
      int references() const           { return lrefs; }
      void incRefs()                   { ++lrefs;  }
      void decRefs()                   { --lrefs;  }
      friend class WaveEvent;
      };

//---------------------------------------------------------
//   Clip
//---------------------------------------------------------

class Clip {
      ClipBase* clip;

   public:
      Clip();
      Clip(ClipBase* clip);
      Clip(const SndFileR& f, int start, int len);
      Clip(const Clip&);
      Clip& operator=(const Clip&);
      bool operator==(const Clip& c) const { return clip == c.clip; }
      bool operator==(ClipBase* c) const { return clip == c; }
      ~Clip();

      bool isNull() const              { return clip == 0; }
      int getRefCount() const          { return clip->getRefCount(); }

      const QString& name() const      { return clip->name();  }
      void setName(const QString& s)   { clip->setName(s); }
      int spos() const                 { return clip->spos();  }
      void setSpos(int s)              { clip->setSpos(s);     }
      SndFileR file1() const           { return clip->file1(); }

      void read(unsigned off, float** f, int ch, unsigned nn) {
            clip->read(off, f, ch, nn);
            }
      int samples() const              { return clip->samples();    }
      void setSamples(int s)           { clip->setSamples(s);       }
      int references() const           { return clip->references(); }
      void incRefs()                   { clip->incRefs();  }
      void decRefs()                   { clip->decRefs();  }
      };

//---------------------------------------------------------
//   ClipList
//---------------------------------------------------------

class ClipList : public std::list<ClipBase*> {
   public:
      int idx(const Clip&) const;
      Clip search(const QString&) const;
      void write(int, Xml&) const;
      void add(ClipBase* clip) { push_back(clip); }
      void remove(ClipBase*);
      };

typedef ClipList::iterator iClip;
typedef ClipList::const_iterator ciClip;
extern ClipBase* readClip(Xml& xml);
#endif

} // namespace MusECore

#endif

