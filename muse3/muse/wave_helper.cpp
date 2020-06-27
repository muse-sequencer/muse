//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2003-2020 Werner Schweer (ws@seh.de) and others
//
//  wave_helper.cpp
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

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QDateTime>
#include <QMessageBox>

#include "wave_helper.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "part.h"
#include "track.h"
#include "audio.h"

namespace MusECore {

bool sndFileCheckCopyOnWrite(const SndFileR sndFile)
{
  if(sndFile.isNull())
    return false;
    
  QString path_this = sndFile.canonicalPath();
  if(path_this.isEmpty())
    return false;

  bool fwrite = sndFile.isFileWritable();

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
            {
              fprintf(stderr,
                "sndFileCheckCopyOnWrite() Error: Two event ids are the same:%d but their parts:%p, %p are not clones!\n",
                (int)id, p, part);
            }
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

void sndFileApplyUndoFile(const Event& original, const QString* tmpfile, unsigned startframe, unsigned endframe)
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
            fprintf(stderr, "sndFileApplyUndoFile: Internal error: original event is empty - Aborting\n");
            return;
            }

      SndFileR orig = original.sndFile();

      if (orig.isNull()) {
            fprintf(stderr, "sndFileApplyUndoFile: Internal error: original sound file is NULL - Aborting\n");
            return;
            }
      if (orig.canonicalPath().isEmpty()) {
            fprintf(stderr, "sndFileApplyUndoFile: Error: Original sound file name is empty - Aborting\n");
            return;
            }

      if (!orig.isOpen()) {
            if (orig.openRead()) {
                  fprintf(stderr,
                    "sndFileApplyUndoFile: Cannot open original file %s for reading - cannot undo! Aborting\n", 
                    orig.canonicalPath().toLocal8Bit().constData());
                  return;
                  }
            }

      SndFile tmp  = SndFile(*tmpfile);
      if (!tmp.isOpen()) {
            if (tmp.openRead()) {
                  fprintf(stderr,
                    "sndFileApplyUndoFile: Could not open temporary file %s for writing - cannot undo! Aborting\n",
                    tmpfile->toLocal8Bit().constData());
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
            fprintf(stderr,
              "sndFileApplyUndoFile: Cannot open orig for write - aborting.\n");
            return;
            }

      orig.seek(startframe, 0);
      orig.write(file_channels, tmpfiledata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);

      // Delete dataholder for temporary file
      for (unsigned i=0; i<file_channels; i++) {
            delete[] tmpfiledata[i];
            }

      // Write the overwritten data to the tmpfile
      if (tmp.openWrite()) {
            fprintf(stderr,
              "sndFileApplyUndoFile: Cannot open tmpfile for writing - redo operation of this file won't be possible. Aborting.\n");
            MusEGlobal::audio->msgIdle(false);
            return;
            }
      tmp.seek(0, 0);
      tmp.write(file_channels, data2beoverwritten, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
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
//   sndFileGetWave
//   If audioConverterSettings and stretchList are given, they are assigned.
//---------------------------------------------------------

SndFileR sndFileGetWave(const QString& inName, bool readOnlyFlag, bool openFlag, bool showErrorBox, 
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

      // Allow multiple instances.
      SndFile* f = nullptr;
      if (!QFile::exists(name)) {
            fprintf(stderr, "wave file <%s> not found\n",
                name.toLocal8Bit().constData());
            return nullptr;
            }
      f = new SndFile(name);

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
              fprintf(stderr, "open wave file(%s) for %s failed: %s\n",
                name.toLocal8Bit().constData(),
                readOnlyFlag ? "writing" : "reading",
                f->strerror().toLocal8Bit().constData());
                if(showErrorBox)
                  QMessageBox::critical(nullptr, QObject::tr("MusE import error."),
                                  QObject::tr("MusE failed to import the file.\n"
                                  "Possibly this wasn't a sound file?\n"
                                  "If it was check the permissions, MusE\n"
                                  "sometimes requires write access to the file."),
                                  QMessageBox::Ok, QMessageBox::Ok);

              delete f;
              f = nullptr;
              }
        }
      return f;
      }

} // namespace MusECore
