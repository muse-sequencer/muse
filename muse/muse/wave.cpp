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

#include <samplerate.h>

#include "al/xml.h"
#include "al/al.h"
#include "song.h"
#include "wave.h"
#include "muse.h"
#include "widgets/filedialog.h"
#include "arranger/arranger.h"
#include "globals.h"
#include "event.h"
#include "audio.h"
#include "part.h"

const char* audioFilePattern[] = {
      "Wave/Binary (*.wav *.bin)",
      "Wave (*.wav)",
      "Binary (*.bin)",
      "All Files (*)",
      0
      };
const int cacheMag = 128;

QHash<QString, SndFile*> SndFile::sndFiles;
QList<SndFile*> SndFile::createdFiles;
int SndFile::recFileNumber;

//---------------------------------------------------------
//   copyWaveFileToProject
//    - copy wave file to project directory
//    - do sample rate conversion
//
//    return false on error
//---------------------------------------------------------

static bool copyWaveFileToProject(const QString& path)
      {
      QFile srcFile(path);
      QFileInfo srcInfo(srcFile);

      QString dst(song->absoluteProjectPath());
      QFile dstFile(dst + "/" + srcInfo.fileName());
      if (dstFile.exists()) {
            // TODO: rename file or check for identity
            //       we cannot easily check for identity if destination
            //       file is sample rate converted
            //
            // for now we assume that the file has already 
            // be imported:
            // 
            printf("File already exists\n");
            return true;
            }

      SF_INFO sfinfoSrc;
      memset(&sfinfoSrc, 0, sizeof(SF_INFO));
      SNDFILE* sfSrc = sf_open(path.toLatin1().data(), SFM_READ, &sfinfoSrc);
      if (sfSrc == 0) {
            printf("Cannot open source file: %s\n", strerror(errno));
            return false;
            }
      int channels         = sfinfoSrc.channels;
      sf_count_t size      = sfinfoSrc.frames;

      SF_INFO sfinfoDst;
      memset(&sfinfoDst, 0, sizeof(SF_INFO));
      sfinfoDst.samplerate = AL::sampleRate;
      sfinfoDst.channels   = channels;
      sfinfoDst.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
      SNDFILE* sfDst       = sf_open(dstFile.fileName().toLatin1().data(),
            SFM_WRITE, &sfinfoDst);
      if (sfDst == 0) {
            printf("Cannot open destination file<%s>: %s\n", 
               dstFile.fileName().toLatin1().data(), strerror(errno));
            return false;
            }

      bool showProgress = size > (1024LL * 1024LL * 8LL);
      QProgressDialog* progress = 0;
      if (showProgress) {
            QString label(QWidget::tr("copy \n  %1\nto\n  %2"));
            label = label.arg(path).arg(dst);
            if (sfinfoSrc.samplerate != AL::sampleRate) {
                  QString cs(QWidget::tr("\nconverting sample rate\n"
                     "from %1 to %2"));
                  label += cs.arg(sfinfoSrc.samplerate).arg(AL::sampleRate);
                  }
            int csize = size / 1024;
            progress = new QProgressDialog(label, QWidget::tr("Abort"), 0, csize);
            progress->setValue(0);
            progress->setWindowTitle("MusE");
            progress->raise();
            progress->show();
            qApp->processEvents();
            }
      sf_count_t inSize = 1024LL * 64LL;
      sf_count_t samplesWritten = 0LL;

      bool returnValue = true;
      if (sfinfoSrc.samplerate != AL::sampleRate) {
            // TODO: convertsample rate
            printf("wave file has samplerate of %d, our project has %d\n", 
               sfinfoSrc.samplerate,AL::sampleRate);

            int srcType = SRC_SINC_MEDIUM_QUALITY;
            int error;
            SRC_STATE* src = src_new(srcType, channels, &error);
            if (src == 0) {
                  printf("creating sample rate converter failed: error %d\n",
                     error);
                  return false;
                  }
            double ratio = double(AL::sampleRate) / double(sfinfoSrc.samplerate);
            src_set_ratio(src, ratio);
            sf_count_t outSize = int(inSize * ratio) + 1;
            float inBuffer[inSize * channels];
            float* inPtr = inBuffer;
            while (size) {
                  float outBuffer[outSize * channels];

                  // read buffer
                  sf_count_t framesToRead = (inBuffer + inSize) - inPtr;
                  if (framesToRead > size)
                        framesToRead = size;
                  sf_count_t nr = sf_readf_float(sfSrc, inPtr, framesToRead);
                  if (nr != framesToRead) {
                        printf("sound file read failed\n");
                        src_delete(src);
                        returnValue = false;
                        break;
                        }

                  // convert
                  SRC_DATA data;
                  data.data_in       = inBuffer;
                  data.data_out      = outBuffer;
                  data.input_frames  = inSize;
                  data.output_frames = outSize;
                  data.end_of_input  = framesToRead == size;
                  data.src_ratio     = ratio;

                  int rv = src_process(src, &data);
                  if (rv > 0) {
                        printf("error sampe rate conversion: %s\n", 
                           src_strerror(rv));
                        src_delete(src);
                        returnValue = false;
                        break;
                        }

                  // write buffer
                  sf_count_t n = sf_writef_float(sfDst, outBuffer, data.output_frames_gen);
                  if (n != data.output_frames_gen) {
                        printf("sound write failed: returns %lld, should return %ld\n", 
                           n, data.output_frames_gen);
                        returnValue = false;
                        break;
                        }

                  int rest = (inPtr + nr - inBuffer) - data.input_frames_used;
                  rest *= (sizeof(float) * channels);
                  if (rest > 0)
                        memcpy(inBuffer, inBuffer + data.input_frames_used, rest);

                  inPtr       += framesToRead;
                  inPtr       -= data.input_frames_used;
                  if (data.input_frames_used > size)
                        size = 0;
                  else
                        size -= data.input_frames_used;
                  framesToRead = data.input_frames_used;
                  samplesWritten += data.input_frames_used;

                  if (framesToRead > size)
                        framesToRead = size;
                  if (showProgress) {
                        progress->setValue(samplesWritten / 1024LL);
                        progress->raise();
                  	qApp->processEvents();
                        if (progress->wasCanceled()) {
                              returnValue = false;
                              break;
                              }
                        }
                  }
            src_delete(src);
            }
      else {
            while (size) {
                  float buffer[inSize * channels];

                  sf_count_t n  = inSize > size ? size : inSize;
                  sf_count_t nn = sf_readf_float(sfSrc, buffer, n);

                  if (nn != n) {
                        printf("sound file read failed\n");
                        returnValue = false;
                        break;
                        }
                  // write buffer
                  nn = sf_writef_float(sfDst, buffer, n);
                  if (n != nn) {
                        printf("sound write failed: returns %lld, should return %lld\n", 
                           nn, n);
                        returnValue = false;
                        break;
                        }
                  size -= n;
                  samplesWritten += n;
                  if (showProgress) {
                        progress->setValue(samplesWritten / 1024LL);
                        progress->raise();
                  	qApp->processEvents();
                        if (progress->wasCanceled()) {
                              returnValue = false;
                              break;
                              }
                        }
                  }
            }
      if (progress)
            delete progress;
      sf_close(sfSrc);
      sf_close(sfDst);
      return returnValue;
      }

//---------------------------------------------------------
//   SndFile
//---------------------------------------------------------

SndFile::SndFile(const QString& name)
      {
      refCount = 0;
      _finfo.setFile(name);
      sfRT  = 0;
      sfUI  = 0;
      csize = 0;
      cache = 0;
      openFlag = false;
      sndFiles[_finfo.absoluteFilePath()] = this;
      }

SndFile::~SndFile()
      {
      if (openFlag)
            close();
      sndFiles.remove(_finfo.absoluteFilePath());
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
	QString p = _finfo.absoluteFilePath();
      sfinfo.format = 0;
      sfUI = sf_open(p.toAscii().data(), SFM_READ, &sfinfo);
      sfinfo.format = 0;
      sfRT = sf_open(p.toAscii().data(), SFM_READ, &sfinfo);
      if (sfUI == 0 || sfRT == 0) {
            printf("SndFile::openRead(): failed\n");
            return true;
            }
      writeFlag = false;
      openFlag  = true;
      QString cacheName = _finfo.absolutePath() + QString("/") + _finfo.baseName() + QString(".wca");
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
      QString cacheName = _finfo.absolutePath() +
         QString("/") + _finfo.baseName() + QString(".wca");
      ::remove(cacheName.toLatin1().data());
      if (openRead()) {
            printf("SndFile::openRead(%s) failed: %s\n", _finfo.filePath().toLatin1().data(), strerror().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   readCache
//---------------------------------------------------------

void SndFile::readCache(const QString& path, bool showProgress)
      {
//      printf("readCache %s for %d samples channel %d\n",
//         path.toLatin1().data(), samples(), channels());

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

      FILE* cfile = fopen(path.toAscii().data(), "r");
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
            label += _finfo.baseName();
            progress = new QProgressDialog(label, "Abort", 0, csize, 0);
            qApp->processEvents();
            }
      float data[channels()][cacheMag];
      float* fp[channels()];
      for (unsigned k = 0; k < channels(); ++k)
            fp[k] = &data[k][0];
      int interval = csize / 10;
      for (int i = 0; i < csize; i++) {
            if (showProgress && ((i % interval) == 0)) {
                  progress->setValue(i);
                  progress->raise();
            	qApp->processEvents();
                  if (progress->wasCanceled()) {
                        // TODO
                        }
                  }
            seek(i * cacheMag);
            read(channels(), fp, cacheMag);
            for (unsigned ch = 0; ch < channels(); ++ch) {
                  float rms = 0.0;
                  cache[ch][i].peak = 0;
                  for (int n = 0; n < cacheMag; n++) {
                        float fd = data[ch][n];
                        rms += fd * fd;
                        int idata = lrint(fd * 255.0);
                        if (idata < 0)
                              idata = -idata;
                        if (idata > 255)
                              idata = 255;
                        if (cache[ch][i].peak < idata)
                              cache[ch][i].peak = idata;
                        }
                  // amplify rms value +12dB
                  int rmsValue = lrint((sqrt(rms/cacheMag) * 255.0));
                  if (rmsValue > 255)
                        rmsValue = 255;
                  cache[ch][i].rms = rmsValue;
                  }
            }
      if (showProgress) {
            progress->setValue(csize);
		qApp->processEvents();
            }
      writeCache(path);
      if (showProgress)
            delete progress;
      }

//---------------------------------------------------------
//   writeCache
//---------------------------------------------------------

void SndFile::writeCache(const QString& path)
      {
      FILE* cfile = fopen(path.toLatin1().data(), "w");
      if (cfile == 0)
            return;
      for (unsigned ch = 0; ch < channels(); ++ch)
            fwrite(cache[ch], csize * sizeof(SampleV), 1, cfile);
      fclose(cfile);
      }

//---------------------------------------------------------
//   read
//    integrate "mag" samples, starting at position "pos"
//    into "s"
//---------------------------------------------------------

void SndFile::read(SampleV* s, int mag, unsigned pos)
      {
      for (unsigned ch = 0; ch < channels(); ++ch) {
            s[ch].peak = 0;
            s[ch].rms  = 0;
            }
      if (pos > samples())
            return;

      if (mag < cacheMag) {
            float data[channels()][mag];
            float* fp[channels()];
            for (unsigned i = 0; i < channels(); ++i)
                  fp[i] = &data[i][0];
            sf_seek(sfUI, pos, SEEK_SET);
            {
            int srcChannels = channels();
            int dstChannels = sfinfo.channels;
            float** dst     = fp;
            float buffer[mag * dstChannels];
            size_t rn  = sf_readf_float(sfUI, buffer, mag);
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
                  s[ch].rms = rms / mag;
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
	QString p = _finfo.filePath();
      sfRT = sf_open(p.toLatin1().data(), SFM_RDWR, &sfinfo);
      sfUI = 0;
      if (sfRT) {
            openFlag  = true;
            writeFlag = true;
            QString cacheName = _finfo.absolutePath() +
               QString("/") + _finfo.baseName() + QString(".wca");
            readCache(cacheName, true);
            }
      return sfRT == 0;
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
      sf_close(sfRT);
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
      QString cacheName = _finfo.absolutePath() + QString("/") + _finfo.baseName() + QString(".wca");
      // QFile::remove(_finfo.filePath());
      // QFile::remove(cacheName);
      // DEBUG:
      QFile::rename(_finfo.filePath(), _finfo.filePath() + ".del");
      QFile::rename(cacheName, cacheName + ".del");
      }

//---------------------------------------------------------
//   samples
//---------------------------------------------------------

unsigned SndFile::samples() const
      {
      return sfinfo.frames;
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

unsigned SndFile::channels() const
      {
      return sfinfo.channels;
      }

//---------------------------------------------------------
//   samplerate
//---------------------------------------------------------

unsigned SndFile::samplerate() const
      {
      return sfinfo.samplerate;
      }

//---------------------------------------------------------
//   format
//---------------------------------------------------------

unsigned SndFile::format() const
      {
      return sfinfo.format;
      }

//---------------------------------------------------------
//   setFormat
//---------------------------------------------------------

void SndFile::setFormat(int fmt, int ch, int rate)
      {
      sfinfo.samplerate = rate;
      sfinfo.channels   = ch;
      sfinfo.format     = fmt;
      sfinfo.seekable   = true;
      sfinfo.frames     = 0;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

size_t SndFile::read(int srcChannels, float** dst, size_t n)
      {
      float buffer[n * sfinfo.channels];
      size_t rn       = sf_readf_float(sfRT, buffer, n);

      float* src      = buffer;
      int dstChannels = sfinfo.channels;

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
      else {
            printf("SndFile:read channel mismatch %d -> %d\n",
               srcChannels, dstChannels);
            }
      return rn;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

size_t SndFile::write(int srcChannels, float** src, size_t n)
      {
      int dstChannels = sfinfo.channels;
      float buffer[n * dstChannels];
      float* dst      = buffer;

      if (srcChannels == dstChannels) {
            for (size_t i = 0; i < n; ++i) {
                  for (int ch = 0; ch < dstChannels; ++ch)
                        *dst++ = *(src[ch]+i);
                  }
            }
      else if ((srcChannels == 1) && (dstChannels == 2)) {
            // mono to stereo
            for (size_t i = 0; i < n; ++i) {
                  float data =  *(src[0]+i);
                  *dst++ = data;
                  *dst++ = data;
                  }
            }
      else if ((srcChannels == 2) && (dstChannels == 1)) {
            // stereo to mono
            for (size_t i = 0; i < n; ++i)
                  *dst++ = *(src[0]+i) + *(src[1]+i);
            }
      else {
            printf("SndFile:write channel mismatch %d -> %d\n",
               srcChannels, dstChannels);
            return 0;
            }
      return sf_writef_float(sfRT, buffer, n) ;
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

off_t SndFile::seek(off_t frames)
      {
      return sf_seek(sfRT, frames, SEEK_SET);
      }

//---------------------------------------------------------
//   strerror
//---------------------------------------------------------

QString SndFile::strerror() const
      {
      char buffer[128];
      buffer[0] = 0;
      sf_error_str(sfRT, buffer, 128);
      return QString(buffer);
      }

//---------------------------------------------------------
//   getWave
//---------------------------------------------------------

SndFile* SndFile::getWave(const QString& inName, bool writeFlag)
      {
      QString name = song->absoluteProjectPath() + "/" + inName;

      SndFile* f = sndFiles.value(name);
// printf("SndFile::getWave: %p writeFlag %d %s %s\n", f, writeFlag, inName.toLatin1().data(), name.toLatin1().data());
      if (f == 0) {
            if (!QFile::exists(name)) {
                  fprintf(stderr, "wave file <%s> not found\n",
                     name.toLatin1().data());
                  return 0;
                  }
            f = new SndFile(name);
            bool error;
            if (writeFlag)
                  error = f->openWrite();
            else
                  error = f->openRead();
            if (error) {
                  fprintf(stderr, "open wave file(%s) for %s failed: %s\n",
                     name.toLatin1().data(),
                     writeFlag ? "writing" : "reading",
                     f->strerror().toLatin1().data());
                  delete f;
                  f = 0;
                  }
            }
      else {
            if (writeFlag && ! f->isWritable()) {
                  if (f->isOpen())
                        f->close();
                  f->openWrite();
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

      //printf("Applying undofile: orig=%s tmpfile=%s startframe=%d endframe=%d\n", original.toLatin1().data(), tmpfile.toLatin1().data(), startframe, endframe);
      SndFile* orig = sndFiles.value(original);
      SndFile tmp  = SndFile(tmpfile);
      if (!orig) {
            printf("Internal error: could not find original file: %s in filelist - Aborting\n", original.toLatin1().data());
            return;
            }

      if (!orig->isOpen()) {
            if (orig->openRead()) {
                  printf("Cannot open original file %s for reading - cannot undo! Aborting\n", original.toLatin1().data());
                  return;
                  }
            }

      if (!tmp.isOpen()) {
            if (tmp.openRead()) {
                  printf("Could not open temporary file %s for writing - cannot undo! Aborting\n", tmpfile.toLatin1().data());
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
      orig->seek(startframe);
      orig->read(file_channels, data2beoverwritten, tmpdatalen);

      orig->close();

      // Read data from temporary file to memory
      float* tmpfiledata[file_channels];
      for (unsigned i=0; i<file_channels; i++) {
            tmpfiledata[i] = new float[tmpdatalen];
            }
      tmp.seek(0);
      tmp.read(file_channels, tmpfiledata, tmpdatalen);
      tmp.close();

      // Write temporary data to original file:
      if (orig->openWrite()) {
            printf("Cannot open orig for write - aborting.\n");
            return;
            }

      orig->seek(startframe);
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
      tmp.seek(0);
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
              tr("to import a audio file you have first to select"
              "a wave track"));
            return;
            }
      QStringList pattern;
      const char** p = audioFilePattern;
      while (*p)
            pattern << *p++;
      QString fn = getOpenFileName(lastWavePath, pattern, this,
         tr("Import Wave File"));
      if (!fn.isEmpty()) {
            QFileInfo qf(fn);
            lastWavePath = qf.path();
            importWave(fn);
            }
      }

//---------------------------------------------------------
//   importWave
//---------------------------------------------------------

bool MusE::importWave(const QString& name)
      {
      return importWaveToTrack(name, (WaveTrack*)(arranger->curTrack()),
         song->cPos());
      }

//---------------------------------------------------------
//   importWaveToTrack
//---------------------------------------------------------

bool MusE::importWaveToTrack(const QString& wave, Track* track, const Pos& pos)
      {
      if (!copyWaveFileToProject(wave))
            return true;
      QFileInfo srcInfo(wave);
      SndFile* f = SndFile::getWave(srcInfo.fileName(), false);

      if (f == 0) {
            printf("import audio file failed\n");
            return true;
            }
      int samples = f->samples();
      track->setChannels(f->channels());

      Part* part = new Part((WaveTrack *)track);
      part->ref();
      part->setType(AL::FRAMES);
      part->setTick(pos.tick());
      part->setLenFrame(samples);

      Event event(Wave);
      SndFileR sf(f);
      event.setSndFile(sf);
      event.setSpos(0);
      event.setLenFrame(samples);
      part->addEvent(event);

      part->setName(srcInfo.baseName());
      song->cmdAddPart(part);
      unsigned endTick = part->tick() + part->lenTick();
      if (song->len() < endTick)
            song->setLen(endTick);
      return false;
      }

//---------------------------------------------------------
//   cmdChangeWave
//   called from GUI context
//---------------------------------------------------------

void Song::cmdChangeWave(QString original, QString tmpfile, unsigned sx, unsigned ex)
      {
      char* original_charstr = new char[original.length() + 1];
      char* tmpfile_charstr = new char[tmpfile.length() + 1];
      strcpy(original_charstr, original.toLatin1().data());
      strcpy(tmpfile_charstr, tmpfile.toLatin1().data());
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
      if (sf && --(sf->refCount) == 0)
            delete sf;
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
      if (sf && --(sf->refCount) <= 0)
            delete sf;
      }

//---------------------------------------------------------
//   createRecFile
//   create soundfile for recording
//---------------------------------------------------------

SndFile* SndFile::createRecFile(int channels)
	{
      QString fileName("%1/rec%2.wav");
      QFileInfo fi;
      do {
		fi.setFile(fileName.arg(song->absoluteProjectPath()).arg(recFileNumber));
            ++recFileNumber;
            } while (fi.exists());
	SndFile* recFile = new SndFile(fi.absoluteFilePath());
      recFile->setFormat(SF_FORMAT_WAV | SF_FORMAT_FLOAT, channels,
         AL::sampleRate);
      createdFiles.append(recFile);
      return recFile;
      }

//---------------------------------------------------------
//   cleanupRecFiles
//	remove all record files which are not referenced
//	any more
//	this is called on exit
//---------------------------------------------------------

void SndFile::cleanupRecFiles(bool removeAll)
      {
      QList<SndFile*>* fl;
      QList<SndFile*> removeFiles;

      if (removeAll)
            fl = &createdFiles;
      else {
            foreach (SndFile* s, createdFiles) {
                  bool remove = true;
      	      WaveTrackList* wt = song->waves();
      	      for (iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt) {
            	      WaveTrack* t = *iwt;
                        PartList* parts = t->parts();
                        for (iPart ip = parts->begin(); ip != parts->end(); ++ip) {
                              Part* part = ip->second;
                              EventList* events = part->events();
                              for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
      	      			if (ie->second.sndFile() == s) {
                                          remove = false;
                                          break;
                                          }
                                    }
                              }
                  	if (t->recFile() && t->recFile()->samples() == 0) {
      	                  t->recFile()->remove();
            	            }
                  	}
                  if (remove)
                        removeFiles.append(s);
                  }
            fl = &removeFiles;
            }
      foreach (SndFile* sf, *fl) {
            printf("cleanup rec file <%s>\n", sf->finfo()->absoluteFilePath().toLatin1().data());
		sf->remove();
      	}
      createdFiles.clear();
      }

//---------------------------------------------------------
//   updateRecFiles
//	this is called on "save"
//	remove all saved wave files from list of potentially
//	to delete files
//---------------------------------------------------------

void SndFile::updateRecFiles()
      {
      QList<SndFile*> removeFiles;

      foreach (SndFile* s, createdFiles) {
            bool remove = true;
	      WaveTrackList* wt = song->waves();
	      for (iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt) {
      	      WaveTrack* t = *iwt;
                  PartList* parts = t->parts();
                  for (iPart ip = parts->begin(); ip != parts->end(); ++ip) {
                        Part* part = ip->second;
                        EventList* events = part->events();
                        for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
	      			if (ie->second.sndFile() == s) {
                                    remove = false;
                                    break;
                                    }
                              }
                        if (!remove)
                              break;
                        }
            	if (t->recFile() && t->recFile()->samples() == 0) {
	                  t->recFile()->remove();
      	            }
                  if (!remove)
                  	break;
            	}
            if (remove)
                  removeFiles.append(s);
            }
      createdFiles = removeFiles;
      }

