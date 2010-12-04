//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wave.cpp,v 1.19.2.20 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cmath>

#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>

#include "xml.h"
#include "song.h"
#include "wave.h"
#include "app.h"
#include "filedialog.h"
#include "arranger/arranger.h"
#include "globals.h"
#include "event.h"
#include "audio.h"
#include "sig.h"

//#define WAVE_DEBUG
//#define WAVE_DEBUG_PRC

/*
const char* audioFilePattern[] = {
      "Wave/Binary (*.wav *.ogg *.bin)",
      "Wave (*.wav *.ogg)",
      "Binary (*.bin)",
      "All Files (*)",
      0
      };
*/
const int cacheMag = 128;

// ClipList* waveClips;

SndFileList SndFile::sndFiles;

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

SndFile::SndFile(const QString& name)
      {
      finfo = new QFileInfo(name);
      sf    = 0;
      sfUI  = 0;
      csize = 0;
      cache = 0;
      openFlag = false;
      sndFiles.push_back(this);
      refCount=0;
      }

SndFile::~SndFile()
      {
      if (openFlag)
            close();
      for (iSndFile i = sndFiles.begin(); i != sndFiles.end(); ++i) {
            if (*i == this) {
                  sndFiles.erase(i);
                  break;
                  }
            }
      delete finfo;
      if (cache) {
            for (unsigned i = 0; i < channels(); ++i)
                  delete cache[i];
            delete[] cache;
            cache = 0;
            }
      }

//---------------------------------------------------------
//   openRead
//---------------------------------------------------------

bool SndFile::openRead()
      {
      if (openFlag) {
            printf("SndFile:: alread open\n");
            return false;
            }
      QString p = path();
      sfinfo.format = 0;
      sf = sf_open(p.latin1(), SFM_READ, &sfinfo);
      sfinfo.format = 0;
      sfUI = sf_open(p.latin1(), SFM_READ, &sfinfo);
      if (sf == 0 || sfUI == 0)
            return true;
            
      writeFlag = false;
      openFlag  = true;
      QString cacheName = finfo->dirPath(true) + QString("/") + finfo->baseName(true) + QString(".wca");
      readCache(cacheName, true);
      return false;
      }

//---------------------------------------------------------
//   update
//    called after recording to file
//---------------------------------------------------------

void SndFile::update()
      {
      close();

      // force recreation of wca data
      QString cacheName = finfo->dirPath(true) +
         QString("/") + finfo->baseName(true) + QString(".wca");
      ::remove(cacheName.latin1());
      if (openRead()) {
            printf("SndFile::update openRead(%s) failed: %s\n", path().latin1(), strerror().latin1());
            }
      }

//---------------------------------------------------------
//   readCache
//---------------------------------------------------------

void SndFile::readCache(const QString& path, bool showProgress)
      {
//      printf("readCache %s for %d samples channel %d\n",
//         path.latin1(), samples(), channels());

      if (cache) {
            for (unsigned i = 0; i < channels(); ++i)
                  delete cache[i];
            delete[] cache;
            }
      if (samples() == 0) {
//            printf("SndFile::readCache: file empty\n");
            return;
            }
      csize = (samples() + cacheMag - 1)/cacheMag;
      cache = new SampleV*[channels()];
      for (unsigned ch = 0; ch < channels(); ++ch)
            cache[ch] = new SampleV[csize];

      FILE* cfile = fopen(path.latin1(), "r");
      if (cfile) {
            for (unsigned ch = 0; ch < channels(); ++ch)
                  fread(cache[ch], csize * sizeof(SampleV), 1, cfile);
            fclose(cfile);
            return;
            }

      //---------------------------------------------------
      //  create cache
      //---------------------------------------------------
      QProgressDialog* progress = 0;
      if (showProgress) {
            QString label(QWidget::tr("create peakfile for "));
            label += basename();
            progress = new QProgressDialog(label,
               QString::null, 0, csize, 0);
            progress->setMinimumDuration(0);
            progress->show();
            }
      float data[channels()][cacheMag];
      float* fp[channels()];
      for (unsigned k = 0; k < channels(); ++k)
            fp[k] = &data[k][0];
      int interval = csize / 10;
      
      if(!interval)
        interval = 1;
      for (int i = 0; i < csize; i++) {
            if (showProgress && ((i % interval) == 0))
                  progress->setValue(i);
            seek(i * cacheMag, 0);
            read(channels(), fp, cacheMag);
            for (unsigned ch = 0; ch < channels(); ++ch) {
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
      writeCache(path);
      if (showProgress)
            delete progress;
      }

//---------------------------------------------------------
//   writeCache
//---------------------------------------------------------

void SndFile::writeCache(const QString& path)
      {
      FILE* cfile = fopen(path.latin1(), "w");
      if (cfile == 0)
            return;
      for (unsigned ch = 0; ch < channels(); ++ch)
            fwrite(cache[ch], csize * sizeof(SampleV), 1, cfile);
      fclose(cfile);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void SndFile::read(SampleV* s, int mag, unsigned pos, bool overwrite)
      {
      if(overwrite)
        for (unsigned ch = 0; ch < channels(); ++ch) {
            s[ch].peak = 0;
            s[ch].rms = 0;
            }
            
      if (pos > samples()) {
//            printf("%p pos %d > samples %d\n", this, pos, samples());
            return;
            }

      if (mag < cacheMag) {
            float data[channels()][mag];
            float* fp[channels()];
            for (unsigned i = 0; i < channels(); ++i)
                  fp[i] = &data[i][0];
            
            sf_count_t ret = 0;
            if(sfUI)
              ret = sf_seek(sfUI, pos, SEEK_SET);
            else
              ret = sf_seek(sf, pos, SEEK_SET);
            if(ret == -1)
              return;  
            {
            int srcChannels = channels();
            int dstChannels = sfinfo.channels;
            size_t n        = mag;
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

            for (unsigned ch = 0; ch < channels(); ++ch) {
                  
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

            for (unsigned ch = 0; ch < channels(); ++ch) {
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
//   openWrite
//---------------------------------------------------------

bool SndFile::openWrite()
      {
      if (openFlag) {
            printf("SndFile:: alread open\n");
            return false;
            }
  QString p = path();
      sf = sf_open(p.latin1(), SFM_RDWR, &sfinfo);
      sfUI = 0;
      if (sf) {
            openFlag  = true;
            writeFlag = true;
            QString cacheName = finfo->dirPath(true) +
               QString("/") + finfo->baseName(true) + QString(".wca");
            readCache(cacheName, true);
            }
      return sf == 0;
      }

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SndFile::close()
      {
      if (!openFlag) {
            printf("SndFile:: alread closed\n");
            return;
            }
      sf_close(sf);
      if (sfUI)
            sf_close(sfUI);
      openFlag = false;
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
      return finfo->baseName(true);
      }

QString SndFile::path() const
      {
      return finfo->filePath();
      }

QString SndFile::dirPath() const
      {
      return finfo->dirPath(true);
      }

QString SndFile::name() const
      {
      return finfo->fileName();
      }

//---------------------------------------------------------
//   samples
//---------------------------------------------------------

unsigned SndFile::samples() const
      {
      if (!writeFlag) // if file is read only sfinfo is reliable
          return sfinfo.frames;
      sf_count_t curPos = sf_seek(sf, 0, SEEK_CUR);
      int frames = sf_seek(sf, 0, SEEK_END);
      sf_seek(sf, curPos, SEEK_SET);
      return frames;
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

unsigned SndFile::channels() const
      {
      return sfinfo.channels;
      }

unsigned SndFile::samplerate() const
      {
      return sfinfo.samplerate;
      }

unsigned SndFile::format() const
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
      delete buffer;
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
            printf("SndFile:read channel mismatch %d -> %d\n",
               srcChannels, dstChannels);
            }

      return rn;

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
      int dstChannels = sfinfo.channels;
      //float buffer[n * dstChannels];
      float *buffer = new float[n * dstChannels];
      float *dst      = buffer;

      const float limitValue=0.9999;


      if (srcChannels == dstChannels) {
            for (size_t i = 0; i < n; ++i) {
                  for (int ch = 0; ch < dstChannels; ++ch)
                        //*dst++ = *(src[ch]+i); // < limitValue ? *(src[ch]+i) : limitValue;
                        if (*(src[ch]+i) > 0)
                          *dst++ = *(src[ch]+i) < limitValue ? *(src[ch]+i) : limitValue;
                        else
                          *dst++ = *(src[ch]+i) > -limitValue ? *(src[ch]+i) : -limitValue;
                  }
            }
      else if ((srcChannels == 1) && (dstChannels == 2)) {
            // mono to stereo
            for (size_t i = 0; i < n; ++i) {
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
            for (size_t i = 0; i < n; ++i)
                  if (*(src[0]+i) + *(src[1]+i) > 0)
                    *dst++ = (*(src[0]+i) + *(src[1]+i)) < limitValue ? (*(src[0]+i) + *(src[1]+i)) : limitValue;
                  else
                    *dst++ = (*(src[0]+i) + *(src[1]+i)) > -limitValue ? (*(src[0]+i) + *(src[1]+i)) : -limitValue;
            }
      else {
            printf("SndFile:write channel mismatch %d -> %d\n",
               srcChannels, dstChannels);
            delete buffer;
            return 0;
            }
      int nbr = sf_writef_float(sf, buffer, n) ;
      delete buffer;
      return nbr;
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

off_t SndFile::seek(off_t frames, int whence)
      {
      return sf_seek(sf, frames, whence);
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
      return 0;
      }

//---------------------------------------------------------
//   getSnd
//---------------------------------------------------------

SndFile* getWave(const QString& inName, bool readOnlyFlag)
      {
      QString name = inName;

      if (QFileInfo(name).isRelative()) {
            name = museProject + QString("/") + name;
            }
      else {
            if (!QFile::exists(name)) {
                  if (QFile::exists(museProject + QString("/") + name)) {
                        name = museProject + QString("/") + name;
                        }
                  }
            }
// printf("=====%s %s\n", inName.latin1(), name.latin1());

      // only open one instance of wave file
      SndFile* f = SndFile::sndFiles.search(name);
      if (f == 0) {
            if (!QFile::exists(name)) {
                  fprintf(stderr, "wave file <%s> not found\n",
                     name.latin1());
                  return 0;
                  }
            f = new SndFile(name);
            bool error;
            if (readOnlyFlag)
                  error = f->openRead();
            else {
                  error = f->openWrite();
                  // if peak cache is older than wave file we reaquire the cache
                  QFileInfo wavinfo(name);
                  QString cacheName = wavinfo.dirPath(true) + QString("/") + wavinfo.baseName(true) + QString(".wca");
                  QFileInfo wcainfo(cacheName);
                  if (!wcainfo.exists() || wcainfo.lastModified() < wavinfo.lastModified()) {
                        //printf("wcafile is older or does not exist!\n");
                        QFile(cacheName).remove();
                        f->readCache(cacheName,true);
                        }
                  
            }
            if (error) {
                  fprintf(stderr, "open wave file(%s) for %s failed: %s\n",
                     name.latin1(),
                     readOnlyFlag ? "writing" : "reading",
                     f->strerror().latin1());
                     QMessageBox::critical(NULL, "MusE import error.", 
                                      "MusE failed to import the file.\n"
                                      "Possibly this wasn't a sound file?\n"
                                      "If it was check the permissions, MusE\n"
                                      "sometimes requires write access to the file.");

                  delete f;
                  f = 0;
                  }
            }
      else {
            if (!readOnlyFlag && ! f->isWritable()) {
                  if (f->isOpen())
                        f->close();
                  f->openWrite();
                  }
            else {
                  // if peak cache is older than wave file we reaquire the cache
                  QFileInfo wavinfo(name);
                  QString cacheName = wavinfo.dirPath(true) + QString("/") + wavinfo.baseName(true) + QString(".wca");
                  QFileInfo wcainfo(cacheName);
                  if (!wcainfo.exists() || wcainfo.lastModified() < wavinfo.lastModified()) {
                        //printf("wcafile is older or does not exist!\n");
                        QFile(cacheName).remove();
                        f->readCache(cacheName,true);
                        }
                  
                  }
            }
      return f;
      }

//---------------------------------------------------------
//   applyUndoFile
//---------------------------------------------------------
void SndFile::applyUndoFile(const QString& original, const QString& tmpfile, unsigned startframe, unsigned endframe)
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

      //printf("Applying undofile: orig=%s tmpfile=%s startframe=%d endframe=%d\n", original.latin1(), tmpfile.latin1(), startframe, endframe);
      SndFile* orig = sndFiles.search(original);
      SndFile tmp  = SndFile(tmpfile);
      if (!orig) {
            printf("Internal error: could not find original file: %s in filelist - Aborting\n", original.latin1());
            return;
            }

      if (!orig->isOpen()) {
            if (orig->openRead()) {
                  printf("Cannot open original file %s for reading - cannot undo! Aborting\n", original.latin1());
                  return;
                  }
            }

      if (!tmp.isOpen()) {
            if (tmp.openRead()) {
                  printf("Could not open temporary file %s for writing - cannot undo! Aborting\n", tmpfile.latin1());
                  return;
                  }
            }

      audio->msgIdle(true);
      tmp.setFormat(orig->format(), orig->channels(), orig->samplerate());

      // Read data in original file to memory before applying tmpfile to original
      unsigned file_channels = orig->channels();
      unsigned tmpdatalen = endframe - startframe;
      float*   data2beoverwritten[file_channels];

      for (unsigned i=0; i<file_channels; i++) {
            data2beoverwritten[i] = new float[tmpdatalen];
            }
      orig->seek(startframe, 0);
      orig->readWithHeap(file_channels, data2beoverwritten, tmpdatalen);

      orig->close();

      // Read data from temporary file to memory
      float* tmpfiledata[file_channels];
      for (unsigned i=0; i<file_channels; i++) {
            tmpfiledata[i] = new float[tmpdatalen];
            }
      tmp.seek(0, 0);
      tmp.readWithHeap(file_channels, tmpfiledata, tmpdatalen);
      tmp.close();

      // Write temporary data to original file:
      if (orig->openWrite()) {
            printf("Cannot open orig for write - aborting.\n");
            return;
            }

      orig->seek(startframe, 0);
      orig->write(file_channels, tmpfiledata, tmpdatalen);

      // Delete dataholder for temporary file
      for (unsigned i=0; i<file_channels; i++) {
            delete[] tmpfiledata[i];
            }

      // Write the overwritten data to the tmpfile
      if (tmp.openWrite()) {
            printf("Cannot open tmpfile for writing - redo operation of this file won't be possible. Aborting.\n");
            audio->msgIdle(false);
            return;
            }
      tmp.seek(0, 0);
      tmp.write(file_channels, data2beoverwritten, tmpdatalen);
      tmp.close();

      // Delete dataholder for replaced original file
      for (unsigned i=0; i<file_channels; i++) {
            delete[] data2beoverwritten[i];
            }

      orig->close();
      orig->openRead();
      orig->update();
      audio->msgIdle(false);
      }

//---------------------------------------------------------
//   importAudio
//---------------------------------------------------------

void MusE::importWave()
      {
      Track* track = arranger->curTrack();
      if (track == 0 || track->type() != Track::WAVE) {
            QMessageBox::critical(this, QString("MusE"),
              tr("to import an audio file you have first to select"
              "a wave track"));
            return;
            }
      //QString fn = getOpenFileName(lastWavePath, audioFilePattern, this,
      QString fn = getOpenFileName(lastWavePath, audio_file_pattern, this,
         tr("Import Wave File"), 0);                                    
      if (!fn.isEmpty()) {
            lastWavePath = fn;
            importWaveToTrack(fn);
            }
      }

//---------------------------------------------------------
//   importWaveToTrack
//---------------------------------------------------------

bool MusE::importWaveToTrack(QString& name, unsigned tick, Track* track)
      {
      if (track==NULL)
            track = (WaveTrack*)(arranger->curTrack());

      SndFile* f = getWave(name, true);

      if (f == 0) {
            printf("import audio file failed\n");
            return true;
            }
      int samples = f->samples();
      if ((unsigned)sampleRate !=f->samplerate()) {
            if(QMessageBox::question(this, tr("Import Wavefile"),
                  tr("This wave file has a samplerate of %1,\n"
                  "as opposed to current setting %2.\n"
                  "Do you still want to import it?").arg(f->samplerate()).arg(sampleRate),
                  tr("&Yes"), tr("&No"),
                  QString::null, 0, 1 ))
                  {
                  //printf("why won't muse let me delete the file object? %d\n", f->getRefCount());
                  if (f->getRefCount() == 0)
                        delete f;
                  return true;
                  }
            }
      track->setChannels(f->channels());

      WavePart* part = new WavePart((WaveTrack *)track);
      if (tick)
          part->setTick(tick);
      else
          part->setTick(song->cpos());
      part->setLenFrame(samples);

      Event event(Wave);
      SndFileR sf(f);
      event.setSndFile(sf);
      event.setSpos(0);
      event.setLenFrame(samples);
      part->addEvent(event);

      part->setName(QFileInfo(name).baseName(true));
      audio->msgAddPart(part);
      unsigned endTick = part->tick() + part->lenTick();
      if (song->len() < endTick)
            song->setLen(endTick);
      return false;
      }
#if 0
//---------------------------------------------------------
//   Clip
//---------------------------------------------------------

ClipBase::ClipBase(const SndFileR& file, int start, int l)
   : f(file)
      {
      refCount = 0;
      for (int i = 1; true; ++i) {
            _name.sprintf("%s.%d", f.basename().latin1(), i);
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
      // waves in the project dirctory are stored
      // with relative path name, others with absolute path
      //
      if (path == museProject)
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
      SndFile* f = 0;
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
         name.latin1());
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

//void Song::cmdAddRecordedWave(WaveTrack* track, const Pos& s, const Pos& e)
void Song::cmdAddRecordedWave(WaveTrack* track, Pos s, Pos e)
      {
      SndFile* f = track->recFile();
      if (f == 0) {
            printf("cmdAddRecordedWave: no snd file for track <%s>\n",
               track->name().latin1());
            return;
            }
      
      // Removed by Tim. p3.3.8
      //unsigned startTick = roundDownBar(s.tick());
      //unsigned endTick   = roundUpBar(e.tick());
      
      // Added by Tim. p3.3.8
      
      if((audio->loopCount() > 0 && s.tick() > lPos().tick()) || (punchin() && s.tick() < lPos().tick()))
        s.setTick(lPos().tick());
      // If we are looping, just set the end to the right marker, since we don't know how many loops have occurred.
      // (Fixed: Added Audio::loopCount)
      // Otherwise if punchout is on, limit the end to the right marker.
      //if(loop() || (punchout() && e.tick() > rPos().tick()) )
      if((audio->loopCount() > 0) || (punchout() && e.tick() > rPos().tick()) )
        e.setTick(rPos().tick());
      // No part to be created? Delete the rec sound file.
      if(s.tick() >= e.tick())
      {
        QString s = f->path();
        delete f;
        // The function which calls this function already does this immediately after. But do it here anyway.
        track->setRecFile(0);
        remove(s.latin1());
        if(debugMsg)
          printf("Song::cmdAddRecordedWave: remove file %s\n", s.latin1());
        return;
      }
      // Round the start down using the Arranger part snap raster value. 
      unsigned startTick = sigmap.raster1(s.tick(), song->recRaster());
      // Round the end up using the Arranger part snap raster value. 
      unsigned endTick   = sigmap.raster2(e.tick(), song->recRaster());

      f->update();

      WavePart* part = new WavePart(track);
      part->setTick(startTick);
      part->setLenTick(endTick - startTick);
      part->setName(track->name());

      // create Event
      Event event(Wave);
      SndFileR sf(f);
      event.setSndFile(sf);
      // We are done with the _recFile member. Set to zero. The function which 
      //  calls this function already does this immediately after. But do it here anyway.
      track->setRecFile(0);
      
      event.setSpos(0);
      
      // Since the part start was snapped down, we must apply the difference so that the
      //  wave event tick lines up with when the user actually started recording.
      // Added by Tim. p3.3.8
      event.setTick(s.tick() - startTick);
      
      
      event.setLenFrame(e.frame() - s.frame());
      part->addEvent(event);

      song->cmdAddPart(part);

      if (song->len() < endTick)
            song->setLen(endTick);
      }

//---------------------------------------------------------
//   cmdChangeWave
//   called from GUI context
//---------------------------------------------------------
void Song::cmdChangeWave(QString original, QString tmpfile, unsigned sx, unsigned ex)
      {
      char* original_charstr = new char[original.length() + 1];
      char* tmpfile_charstr = new char[tmpfile.length() + 1];
      strcpy(original_charstr, original.latin1());
      strcpy(tmpfile_charstr, tmpfile.latin1());
      song->undoOp(UndoOp::ModifyClip, original_charstr, tmpfile_charstr, sx, ex);
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

SndFileR& SndFileR::operator=(const SndFileR& ed)
      {
      if (sf == ed.sf)
            return *this;
      if (sf && --(sf->refCount) == 0) {
            delete sf;
            }
      sf = ed.sf;
      if (sf)
            (sf->refCount)++;
      return *this;
      }

//---------------------------------------------------------
//   ~SndFileR
//---------------------------------------------------------

SndFileR::~SndFileR()
      {
      if (sf)
            if (--(sf->refCount) == 0) {
                delete sf;
                sf=NULL;
                }
      }
