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

#include <QString>

class QFileInfo;

namespace MusECore {

class Event;

//class Xml;

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
// REMOVE Tim. samplerate. Added.
//class AudioConverter;
class AudioConverterPluginI;
class StretchList;
class AudioConverterSettingsGroup;
class PendingOperationList;

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

class SndFile {
      QFileInfo* finfo;
      SNDFILE* sf;
      SNDFILE* sfUI;
      // REMOVE Tim. samplerate. Added.
      AudioConverterPluginI* _staticAudioConverter;
      AudioConverterPluginI* _staticAudioConverterUI;
      AudioConverterPluginI* _dynamicAudioConverter;
      AudioConverterPluginI* _dynamicAudioConverterUI;
      AudioConverterSettingsGroup* _audioConverterSettings;
      StretchList* _stretchList;
      
      SF_INFO sfinfo;
      SampleVtype* cache;
      sf_count_t csize;                    //!< frames in cache

      float *writeBuffer;
      size_t writeSegSize;

      void writeCache(const QString& path);

      bool openFlag;
      bool writeFlag;
      size_t readInternal(int srcChannels, float** dst, size_t n, bool overwrite, float *buffer);
      size_t realWrite(int channel, float**, size_t n, size_t offs = 0);
      // Creates a new converter based on the supplied settings and AudioConverterSettings::ModeType mode.
      // If isLocalSettings is true, settings is treated as a local settings which may override the 
      //  global default settings.
      // If isLocalSettings is false, settings is treated as the global default settings and is 
      //  directly used instead of the comparison to, and possible use of, the global default above.
      AudioConverterPluginI* setupAudioConverter(AudioConverterSettingsGroup* settings, bool isLocalSettings, int mode);
      
   protected:
      int refCount;

   public:
      SndFile(const QString& name);
      ~SndFile();
      int getRefCount() { return refCount; }

      static SndFileList sndFiles;
      static void applyUndoFile(const Event& original, const QString* tmpfile, unsigned sx, unsigned ex);

      void createCache(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart = 0);
      void readCache(const QString& path, bool progress);

      // REMOVE Tim. samplerate. Added.
      // Creates a converted cache if a samplerate or shift/stretch converter is active. Otherwise a normal createCache() is called.
      //void createCacheConverted(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart = 0);
      // Reads a converted cache if a samplerate or shift/stretch converter is active. Otherwise a normal readCache() is called.
      //void readCacheConverted(const QString& path, bool progress);
      
      bool openRead(bool createCache=true, bool showProgress=true);        //!< returns true on error
      bool openWrite();       //!< returns true on error
      void close();
      void remove();

      bool isOpen() const     { return openFlag; }
      bool isWritable() const { return writeFlag; }
      void update(bool showProgress = true);
      bool checkCopyOnWrite();      //!< check if the file should be copied before writing to it

      QString basename() const;     //!< filename without extension
      QString dirPath() const;      //!< path
      QString canonicalDirPath() const; //!< path, resolved (no symlinks or . .. etc)
      QString path() const;         //!< path with filename
      QString canonicalPath() const; //!< path with filename, resolved (no symlinks or . .. etc)
      QString name() const;         //!< filename

      // REMOVE Tim. samplerate. Added.
      // Ratio of the file's sample rate to the current audio sample rate.
      double sampleRateRatio() const;
      // Whether the sample rate ratio is exactly 1.
      bool sampleRateDiffers() const;
      // Convert a frame position to its resampled or stretched position.
      sf_count_t convertPosition(sf_count_t pos) const;
      // Returns whether ANY stretch event has a stretch ratio other than 1.0 
      //  ie. the map is stretched, a stretcher must be engaged.
      bool isStretched() const;
      // Returns whether ANY stretch event has a samplerate ratio other than 1.0 
      //  ie. the map is stretched, a samplerate converter must be engaged.
      bool isResampled() const;
      
      sf_count_t samples() const;
      int channels() const;
      int samplerate() const;
      int format() const;
      int sampleBits() const;
      void setFormat(int fmt, int ch, int rate);

      size_t read(int channel, float**, size_t, bool overwrite = true);
      size_t readWithHeap(int channel, float**, size_t, bool overwrite = true);
      size_t readDirect(float* buf, size_t n)    { return sf_readf_float(sf, buf, n); }
      size_t write(int channel, float**, size_t);
      size_t writeDirect(float *buf, size_t n) { return sf_writef_float(sf, buf, n); }

      // REMOVE Tim. samplerate. Added. For now I must provide separate routines here, don't want to upset anything else.
      // Reads realtime audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      sf_count_t readConverted(sf_count_t pos, int srcChannels, float** buffer, sf_count_t frames, bool overwrite = true);
      // Reads graphical audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      void readConverted(SampleV* s, int mag, sf_count_t pos, bool overwrite = true, bool allowSeek = true);
      // Seeks to a converted position if a samplerate or shift/stretch converter is active. Otherwise a normal seek.
      sf_count_t seekConverted(sf_count_t frames, int whence);
//       AudioConverterPluginI* staticAudioConverter() const { return _staticAudioConverter; }
//       AudioConverterPluginI* staticAudioConverterUI() const { return _staticAudioConverterUI; }
//       AudioConverterPluginI* dynamicAudioConverter() const { return _dynamicAudioConverter; }
//       AudioConverterPluginI* dynamicAudioConverterUI() const { return _dynamicAudioConverterUI; }
      AudioConverterPluginI* staticAudioConverter(int mode) const;
      void setStaticAudioConverter(AudioConverterPluginI* converter, int mode);
      AudioConverterSettingsGroup* audioConverterSettings() const { return _audioConverterSettings; }
      void setAudioConverterSettings(AudioConverterSettingsGroup* settings);
      StretchList* stretchList() const { return _stretchList; }
      
// REMOVE Tim. samplerate. Changed.
//       off_t seek(off_t frames, int whence);
      sf_count_t seek(sf_count_t frames, int whence);
      void read(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true);
      QString strerror() const;

      // If isLocalSettings is true, settings is treated as a local settings which may override the 
      //  global default settings.
      // If isLocalSettings is false, settings is treated as the global default settings and is 
      //  directly used instead of the comparison to, and possible use of, the global default above.
      void modifyAudioConverterOperation(AudioConverterSettingsGroup* settings, bool isLocalSettings, PendingOperationList& ops); 
      
      static SndFile* search(const QString& name);

      friend class SndFileR;
      };

//---------------------------------------------------------
//   SndFileR
//    SndFile with reference count
//---------------------------------------------------------

class SndFileR {
      SndFile* sf;

   public:
      SndFileR() { sf = 0; }
      SndFileR(SndFile* _sf);
      SndFileR(const SndFileR& ed);
      SndFileR& operator=(SndFile* ptr);
      SndFileR& operator=(const SndFileR& ed);
      bool operator==(const SndFileR& c) const { return sf == c.sf; }
      bool operator==(SndFile* c) const { return sf == c; }
      SndFile* operator->() { return sf; }
      const SndFile* operator->() const { return sf; }
      operator bool() { return sf!=NULL; }
      ~SndFileR();
      int getRefCount() const { return sf ? sf->refCount : 0; }
      bool isNull() const     { return sf == 0; }

      bool openRead(bool createCache=true)         { return sf ? sf->openRead(createCache) : true;  }
      bool openWrite()        { return sf ? sf->openWrite() : true; }
      void close()            { if(sf) sf->close();     }
      void remove()           { if(sf) sf->remove();    }

      bool isOpen() const     { return sf ? sf->isOpen() : false; }
      bool isWritable() const { return sf ? sf->isWritable() : false; }
      void update()           { if(sf) sf->update(); }
      bool checkCopyOnWrite() { return sf ? sf->checkCopyOnWrite() : false; }

      QString basename() const { return sf ? sf->basename() : QString(); }
      QString dirPath() const  { return sf ? sf->dirPath() : QString(); }
      QString canonicalDirPath() const  { return sf ? sf->canonicalDirPath() : QString(); }
      QString path() const     { return sf ? sf->path() : QString(); }
      QString canonicalPath() const  { return sf ? sf->canonicalPath() : QString(); }
      QString name() const     { return sf ? sf->name() : QString(); }

      // REMOVE Tim. samplerate. Added.
      // Ratio of the file's sample rate to the current audio sample rate.
      inline double sampleRateRatio() const { return sf ? sf->sampleRateRatio() : 1.0; };
      // Whether the sample rate ratio is exactly 1.
      inline bool sampleRateDiffers() const { return sf ? sf->sampleRateDiffers() : false; };
      // Convert a frame position to its resampled or stretched position.
      inline sf_count_t convertPosition(sf_count_t pos) const { return sf ? sf->convertPosition(pos) : pos; };
      
// REMOVE Tim. samplerate. Changed.
//       unsigned samples() const    { return sf ? sf->samples() : 0; }
      sf_count_t samples() const    { return sf ? sf->samples() : 0; }
      int channels() const   { return sf ? sf->channels() : 0; }
      int samplerate() const { return sf ? sf->samplerate() : 0; }
      int format() const     { return sf ? sf->format() : 0; }
      int sampleBits() const      { return sf ? sf->sampleBits() : 0; }
      void setFormat(int fmt, int ch, int rate) {
            if(sf) sf->setFormat(fmt, ch, rate);
            }
      size_t readWithHeap(int channel, float** f, size_t n, bool overwrite = true) {
            return sf ? sf->readWithHeap(channel, f, n, overwrite) : 0;
            }
      size_t read(int channel, float** f, size_t n, bool overwrite = true) {
            return sf ? sf->read(channel, f, n, overwrite) : 0;
            }
      size_t readDirect(float* f, size_t n) { return sf ? sf->readDirect(f, n) : 0; }  
      
      size_t write(int channel, float** f, size_t n) {
            return sf ? sf->write(channel, f, n) : 0;
            }
            
      // REMOVE Tim. samplerate. Added. For now I must provide separate routines here, don't want to upset anything else.
      // Reads realtime audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      sf_count_t readConverted(sf_count_t pos, int channel, float** buffer, sf_count_t frames, bool overwrite = true) {
            return sf ? sf->readConverted(pos, channel, buffer, frames, overwrite) : 0; }
      // Reads graphical audio converted if a samplerate or shift/stretch converter is active. Otherwise a normal read.
      void readConverted(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true) {
            if(sf) sf->readConverted(s, mag, pos, overwrite, allowSeek); }
      // Seeks to a converted position if a samplerate or shift/stretch converter is active. Otherwise a normal seek.
      sf_count_t seekConverted(sf_count_t frames, int whence) { return sf ? sf->seekConverted(frames, whence) : 0; }
//       AudioConverter* audioConverter() { return sf ? sf->staticAudioConverter() : 0; }
//       AudioConverter* audioConverterUI() { return sf ? sf->staticAudioConverterUI() : 0; }
//       AudioConverterPluginI* staticAudioConverter() const { return sf ? sf->staticAudioConverter() : 0; }
//       AudioConverterPluginI* staticAudioConverterUI() const { return sf ? sf->staticAudioConverterUI() : 0; }
//       AudioConverterPluginI* dynamicAudioConverter() const { return sf ? sf->dynamicAudioConverter() : 0; }
//       AudioConverterPluginI* dynamicAudioConverterUI() const { return sf ? sf->dynamicAudioConverterUI() : 0; }
      AudioConverterPluginI* staticAudioConverter(int mode) const
      { return sf ? sf->staticAudioConverter(mode) : 0; }
      void setStaticAudioConverter(AudioConverterPluginI* converter, int mode)
      { if(sf) sf->setStaticAudioConverter(converter, mode); }
      AudioConverterSettingsGroup* audioConverterSettings() const { return sf ? sf->audioConverterSettings() : 0; }
      void setAudioConverterSettings(AudioConverterSettingsGroup* settings)
      { if(sf) sf->setAudioConverterSettings(settings); }
      StretchList* stretchList() const { return sf ? sf->stretchList() : 0; }
      // If isLocalSettings is true, settings is treated as a local settings which may override the 
      //  global default settings.
      // If isLocalSettings is false, settings is treated as the global default settings and is 
      //  directly used instead of the comparison to, and possible use of, the global default above.
      void modifyAudioConverterOperation(AudioConverterSettingsGroup* settings, bool isLocalSettings, PendingOperationList& ops) 
      { if(sf) sf->modifyAudioConverterOperation(settings, isLocalSettings, ops); }
      
// REMOVE Tim. samplerate. Changed.
//       off_t seek(off_t frames, int whence) {
      sf_count_t seek(sf_count_t frames, int whence) { return sf ? sf->seek(frames, whence) : 0; }
      void read(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true) {
            if(sf) sf->read(s, mag, pos, overwrite, allowSeek);
            }
      QString strerror() const { return sf ? sf->strerror() : QString(); }
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

extern SndFileR getWave(const QString& name, bool readOnlyFlag, bool openFlag = true, bool showErrorBox = true, 
                        AudioConverterSettingsGroup* audioConverterSettings = NULL, StretchList* stretchList = NULL);

} // namespace MusECore

#endif

