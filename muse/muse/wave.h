//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __WAVE_H__
#define __WAVE_H__

#include <sndfile.h>

//---------------------------------------------------------
//   SampleV
//    peak file value
//---------------------------------------------------------

struct SampleV {
      unsigned char peak;
      unsigned char rms;
      };

class SndFile;

class SndFileList : public QHash<QString, SndFile*> {
   public:
      SndFile* search(const QString& name) { return (*this)[name]; }
      };

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

class SndFile {
      static QHash<QString, SndFile*> sndFiles;
      static QList<SndFile*> createdFiles;
      static int recFileNumber;

      QFileInfo _finfo;
      SNDFILE* sfRT;          // used by rt process (prefetch)
      SNDFILE* sfUI;          // used by ui process
      SF_INFO sfinfo;
      SampleV** cache;
      int csize;                    // frames in cache

      void readCache(const QString& path, bool progress);
      void writeCache(const QString& path);

      bool openFlag;
      bool writeFlag;

   protected:
      int refCount;

   public:
      SndFile(const QString& name);
      ~SndFile();

      static void applyUndoFile(const QString& original, const QString& tmpfile, unsigned sx, unsigned ex);

      bool openRead();        // return true on error
      bool openWrite();       // return true on error
      void close();
      void remove();

      bool isOpen() const     { return openFlag; }
      bool isWritable() const { return writeFlag; }
      void update();

      QFileInfo* finfo() 	{ return &_finfo; }

      unsigned samples() const;
      unsigned channels() const;
      unsigned samplerate() const;
      unsigned format() const;
      int sampleBits() const;
      void setFormat(int fmt, int ch, int rate);

      size_t read(int channel, float**, size_t);
      size_t write(int channel, float**, size_t);

      off_t seek(off_t frames);
      void read(SampleV* s, int mag, unsigned pos);
      QString strerror() const;

	static SndFile* createRecFile(int);
      static void cleanupRecFiles(bool);
      static void updateRecFiles();
      static SndFile* search(const QString& name);
	static SndFile* getWave(const QString& inName, bool writeFlag);

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
      SndFileR& operator=(const SndFileR& ed);
      bool operator==(const SndFileR& c) const { return sf == c.sf; }
      bool operator==(SndFile* c) const { return sf == c; }
      ~SndFileR();
      int getRefCount() const { return sf->refCount; }
      bool isNull() const     { return sf == 0; }

      bool openRead()         { return sf->openRead();  }
      bool openWrite()        { return sf->openWrite(); }
      void close()            { sf->close();     }
      void remove()           { sf->remove();    }

      bool isOpen() const     { return sf->isOpen(); }
      bool isWritable() const { return sf->isWritable(); }
      void update()           { sf->update(); }
      QFileInfo* finfo() 	{ return sf->finfo(); }
      const QFileInfo* finfo() const { return sf->finfo(); }

      unsigned samples() const    { return sf->samples(); }
      unsigned channels() const   { return sf->channels(); }
      unsigned samplerate() const { return sf->samplerate(); }
      unsigned format() const     { return sf->format(); }
      int sampleBits() const      { return sf->sampleBits(); }
      void setFormat(int fmt, int ch, int rate) {
            sf->setFormat(fmt, ch, rate);
            }
      size_t read(int channel, float** f, size_t n) {
            return sf->read(channel, f, n);
            }
      size_t write(int channel, float** f, size_t n) {
            return sf->write(channel, f, n);
            }
      off_t seek(off_t frames) {
            return sf->seek(frames);
            }
      void read(SampleV* s, int mag, unsigned pos) {
            sf->read(s, mag, pos);
            }
      QString strerror() const { return sf->strerror(); }
      };

#endif

