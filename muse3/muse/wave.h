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

// REMOVE Tim. wave. Removed.
//class Event;

// REMOVE Tim. wave. Removed.
// class Xml;

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
      
   protected:
      int refCount;

   public:
      SndFile(const QString& name);
      ~SndFile();
      int getRefCount() { return refCount; }

      static SndFileList sndFiles;
// REMOVE Tim. wave. Removed. Moved into helper.cpp
//       static void applyUndoFile(const Event& original, const QString* tmpfile, unsigned sx, unsigned ex);

      void createCache(const QString& path, bool showProgress, bool bWrite, sf_count_t cstart = 0);
      void readCache(const QString& path, bool progress);

      bool openRead(bool createCache=true, bool showProgress=true);        //!< returns true on error
      bool openWrite();       //!< returns true on error
      void close();
      void remove();

      bool isOpen() const     { return openFlag; }
      bool isWritable() const { return writeFlag; }
      void update(bool showProgress = true);
// REMOVE Tim. wave. Removed. Moved into helper.cpp
//       bool checkCopyOnWrite();      //!< check if the file should be copied before writing to it

      QString basename() const;     //!< filename without extension
      QString dirPath() const;      //!< path
      QString canonicalDirPath() const; //!< path, resolved (no symlinks or . .. etc)
      QString path() const;         //!< path with filename
      QString canonicalPath() const; //!< path with filename, resolved (no symlinks or . .. etc)
      QString name() const;         //!< filename

      sf_count_t samples() const;
      unsigned channels() const;
      unsigned samplerate() const;
      unsigned format() const;
      int sampleBits() const;
      void setFormat(int fmt, int ch, int rate);

      size_t read(int channel, float**, size_t, bool overwrite = true);
      size_t readWithHeap(int channel, float**, size_t, bool overwrite = true);
      size_t readDirect(float* buf, size_t n)    { return sf_readf_float(sf, buf, n); }
      size_t write(int channel, float**, size_t);
      size_t writeDirect(float *buf, size_t n) { return sf_writef_float(sf, buf, n); }

      off_t seek(off_t frames, int whence);
      void read(SampleV* s, int mag, unsigned pos, bool overwrite = true, bool allowSeek = true);
      QString strerror() const;

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
// REMOVE Tim. wave. Removed. Moved into helper.cpp
//       bool checkCopyOnWrite() { return sf ? sf->checkCopyOnWrite() : false; }

      QString basename() const { return sf ? sf->basename() : QString(); }
      QString dirPath() const  { return sf ? sf->dirPath() : QString(); }
      QString canonicalDirPath() const  { return sf ? sf->canonicalDirPath() : QString(); }
      QString path() const     { return sf ? sf->path() : QString(); }
      QString canonicalPath() const  { return sf ? sf->canonicalPath() : QString(); }
      QString name() const     { return sf ? sf->name() : QString(); }

      unsigned samples() const    { return sf ? sf->samples() : 0; }
      unsigned channels() const   { return sf ? sf->channels() : 0; }
      unsigned samplerate() const { return sf ? sf->samplerate() : 0; }
      unsigned format() const     { return sf ? sf->format() : 0; }
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
      off_t seek(off_t frames, int whence) {
            return sf ? sf->seek(frames, whence) : 0;
            }
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

// REMOVE Tim. wave. Removed. Moved into helper.cpp
// extern SndFileR getWave(const QString& name, bool readOnlyFlag, bool openFlag = true, bool showErrorBox = true);

} // namespace MusECore

#endif

