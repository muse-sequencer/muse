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

#include <sys/wait.h>

#include "waveview.h"
#include "song.h"
#include "midiedit/midieditor.h"
#include "al/tempo.h"
#include "event.h"
#include "globals.h"
#include "waveedit.h"
#include "audio.h"
#include "gconfig.h"
#include "part.h"

static const int partLabelHeight = 13;

//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

WaveView::WaveView(WaveEdit* pr)
   : TimeCanvas(TIME_CANVAS_WAVEEDIT)
      {
      setMarkerList(song->marker());
      selectionStart = 0;
      selectionStop  = 0;
      lastGainvalue  = 100;
      editor         = pr;

	curPart = editor->parts()->begin()->second;
      setMouseTracking(true);

      songChanged(SC_TRACK_INSERTED);
      }

//---------------------------------------------------------
//   drawWavePart
//    y0 - start of track
//    th - track height
//    from - x pixel coordinate start drawing
//    to   - x end drawing
//
//    redraw area is QRect(from, y0, to-from, th)
//---------------------------------------------------------

void WaveView::drawWavePart(QPainter& p, Part* wp, int y0, int th, int from, int to)
      {
      int h  = th/2;
      int y  = y0 + 1 + h;
      int cc = th % 2 ? 0 : 1;

	const Pos pos(pix2pos(from));
      EventList* el = wp->events();
      for (iEvent e = el->begin(); e != el->end(); ++e) {
            Event event = e->second;
            SndFileR f  = event.sndFile();
            if (f.isNull())
                  continue;
            unsigned channels = f.channels();
            if (channels == 0) {
                  printf("drawWavePart: channels==0! %s\n", f.finfo()->fileName().toLatin1().data());
                  continue;
                  }

            int x1 = pos2pix(event.pos() + *wp);
            int x2 = pos2pix(event.end() + *wp);
            int w  = x2 - x1;
            if (w == 0)
                  continue;

            int samples = event.lenFrame();
            int xScale  = (samples + w/2)/w;
            int frame   = pos.frame() - wp->frame() 
                           - event.pos().frame() + event.spos();

            if (h < 20) {
                  //
                  //    combine multi channels into one waveform
                  //
                  for (int i = from; i < to; i++) {
                        SampleV sa[channels];
                        f.read(sa, xScale, frame);
                        frame += xScale;
                        int peak = 0;
                        int rms  = 0;
                        for (unsigned k = 0; k < channels; ++k) {
                              if (sa[k].peak > peak)
                                    peak = sa[k].peak;
                              rms += sa[k].rms;
                              }
                        rms /= channels;
                        peak = (peak * (th-2)) >> 9;
                        rms  = (rms  * (th-2)) >> 9;
                        p.setPen(QColor(Qt::darkGray));
                        p.drawLine(i, y - peak - cc, i, y + peak);
                        p.setPen(QColor(Qt::black));
                        p.drawLine(i, y - rms - cc, i, y + rms);
                        }
                  }
            else {
                  //
                  //  multi channel display
                  //
                  h = th / (channels * 2);
                  int cc = th % (channels * 2) ? 0 : 1;
                  for (int i = from; i < to; i++) {
                        y  = y0 + 1 + h;
                        SampleV sa[channels];
                        f.read(sa, xScale, frame);
                        frame += xScale;
                        for (unsigned k = 0; k < channels; ++k) {
                              // peak = (sa[k].peak * h) / 256;
                              int peak = (sa[k].peak * (h - 1)) >> 8;
                              int rms  = (sa[k].rms  * (h - 1)) >> 8;
                              p.setPen(QColor(Qt::darkGray));
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(QColor(Qt::black));
                              p.drawLine(i, y - rms - cc, i, y + rms);
                              y  += 2 * h;
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveView::paint(QPainter& p, QRect r)
      {
      QFont f = font();
      f.setPointSize(8);
      p.setFont(f);

      int from = r.x();
      int to   = from + r.width();

      PartList* pl = editor->parts();
      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            Part* part = ip->second;
            int x1  = pos2pix(*part);
            int x2  = pos2pix(part->end());
            int len = x2 - x1;

            if (x2 <= from)
                  continue;
            if (x1 > to)
                  break;

            int h = rCanvasA.height();
            int xx1 = x1;
            if (xx1 < from)
                  xx1 = from;
            int xx2 = x2;
            if (xx2 > to)
                  xx2 = to;
            drawWavePart(p, part, 0, h, xx1, xx2);
            int yy = h - partLabelHeight;
            p.drawText(x1 + 3, yy, len - 6,
               partLabelHeight-1, Qt::AlignVCenter | Qt::AlignLeft,
               part->name());
            }
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString WaveView::getCaption() const
      {
      QString caption("MusE: WaveEditor");
      if (curPart)
            return caption + QString(": ") + curPart->name();
      return caption;
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void WaveView::songChanged(int flags)
      {
      if (flags & ~SC_SELECTION) {
            startFrame  = MAXINT;
            endFrame    = 0;
            for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  Part* part = p->second;
                  int sframe = part->frame();
                  int eframe = sframe + part->lenFrame();
                  if (sframe < startFrame)
                        startFrame = sframe;
                  if (eframe > endFrame)
                        endFrame = eframe;
                  }
            }
//      if (flags & SC_CLIP_MODIFIED) {
//            update(); // Boring, but the only thing possible to do
//            }
      setPart(*curPart, curPart->end());
      widget()->update();
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void WaveView::viewMousePressEvent(QMouseEvent* /*event*/)
      {
#if 0
      button = event->button();
      unsigned x = event->x();

      switch (button) {
            case Qt::LeftButton:
                  if (mode == NORMAL) {
                        // redraw and reset:
                        if (selectionStart != selectionStop) {
                              selectionStart = selectionStop = 0;
                              update();
                              }
                        mode = DRAG;
                        dragstartx = x;
                        selectionStart = selectionStop = x;
                        }
                  break;

            case Qt::MidButton:
            case Qt::RightButton:
            default:
                  break;
            }
      viewMouseMoveEvent(event);
#endif
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void WaveView::viewMouseReleaseEvent(QMouseEvent*)
      {
#if 0
      button = Qt::NoButton;

      if (mode == DRAG) {
            mode = NORMAL;
            //printf("selectionStart=%d selectionStop=%d\n", selectionStart, selectionStop);
            }
#endif
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void WaveView::viewMouseMoveEvent(QMouseEvent* /*event*/)
      {
#if 0
      unsigned x = event->x();
      emit timeChanged(x);

      int i;
      switch (button) {
            case Qt::LeftButton:
                  i = 0;
                  if (mode == DRAG) {
                        if (x < dragstartx) {
                              selectionStart = x;
                              selectionStop = dragstartx;
                              }
                        else {
                              selectionStart = dragstartx;
                              selectionStop = x;
                              }
                        }
                  break;
            case Qt::MidButton:
                  i = 1;
                  break;
            case Qt::RightButton:
                  i = 2;
                  break;
            default:
                  return;
            }
      Pos p(AL::tempomap.frame2tick(x), true);
      song->setPos(i, p);
#endif
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveView::cmd(const QString& c)
      {
#if 0
      int modifyoperation = -1;
      double paramA = 0.0;

      switch(n) {
            case WaveEdit::CMD_SELECT_ALL:
            if (!editor->parts()->empty()) {
                  iPart iBeg = editor->parts()->begin();
                  iPart iEnd = editor->parts()->end();
                  iEnd--;
                  Part* beg = iBeg->second;
                  Part* end = iEnd->second;
                  selectionStart = beg->frame();
                  selectionStop  = end->frame() + end->lenFrame();
                  update();
                  }
                  break;

            case WaveEdit::CMD_EDIT_EXTERNAL:
                  modifyoperation = EDIT_EXTERNAL;
                  break;

            case WaveEdit::CMD_SELECT_NONE:
                  selectionStart = selectionStop = 0;
                  update();
                  break;

            case WaveEdit::CMD_MUTE:
                  modifyoperation = MUTE;
                  break;

            case WaveEdit::CMD_NORMALIZE:
                  modifyoperation = NORMALIZE;
                  break;

            case WaveEdit::CMD_FADE_IN:
                  modifyoperation = FADE_IN;
                  break;

            case WaveEdit::CMD_FADE_OUT:
                  modifyoperation = FADE_OUT;
                  break;

            case WaveEdit::CMD_REVERSE:
                  modifyoperation = REVERSE;
                  break;

            case WaveEdit::CMD_GAIN_FREE: {
            /*
                  EditGain* editGain = new EditGain(this, lastGainvalue);
                  if (editGain->exec() == QDialog::Accepted) {
                        lastGainvalue = editGain->getGain();
                        modifyoperation = GAIN;
                        paramA = (double)lastGainvalue / 100.0;
                        }
                  delete editGain;
                  */
                  printf("Free gain - todo!\n");
                  }
                  break;

            case WaveEdit::CMD_GAIN_200:
                  modifyoperation = GAIN;
                  paramA = 2.0;
                  break;

            case WaveEdit::CMD_GAIN_150:
                  modifyoperation = GAIN;
                  paramA = 1.5;
                  break;

            case WaveEdit::CMD_GAIN_75:
                  modifyoperation = GAIN;
                  paramA = 0.75;
                  break;

            case WaveEdit::CMD_GAIN_50:
                  modifyoperation = GAIN;
                  paramA = 0.5;
                  break;

            case WaveEdit::CMD_GAIN_25:
                  modifyoperation = GAIN;
                  paramA = 0.25;
                  break;

            default:
                  break;
            }

      if (modifyoperation != -1) {
            if (selectionStart == selectionStop) {
                  printf("No selection. Ignoring\n"); //@!TODO: Disable menu options when no selection
                  return;
                  }
            modifySelection(modifyoperation, selectionStart, selectionStop, paramA);
            }
#endif
      }

//---------------------------------------------------------
//   getSelection
//---------------------------------------------------------

WaveSelectionList WaveView::getSelection(unsigned /*startpos*/, unsigned /*stoppos*/)
      {
      WaveSelectionList selection;
#if 0
      for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            Part* wp = ip->second;
            unsigned part_offset = wp->frame();
            EventList* el = wp->events();

            for (iEvent e = el->begin(); e != el->end(); ++e) {
                  Event event  = e->second;
                  if (event.empty())
                        continue;
                  SndFileR file = event.sndFile();
                  if (file.isNull())
                        continue;

                  unsigned event_offset = event.frame() + part_offset;
                  unsigned event_startpos  = event.spos();
                  unsigned event_length = event.lenFrame() + event.spos();
                  unsigned event_end    = event_offset + event_length;
                  //printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n", startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);

                  if (!(event_end <= startpos || event_offset >= stoppos)) {
                        int tmp_sx = startpos - event_offset + event_startpos;
                        int tmp_ex = stoppos  - event_offset + event_startpos;
                        unsigned sx;
                        unsigned ex;

                        tmp_sx < (int)event_startpos ? sx = event_startpos : sx = tmp_sx;
                        tmp_ex > (int)event_length   ? ex = event_length   : ex = tmp_ex;

                        //printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLatin1().data());
                        WaveEventSelection s;
                        s.file = file;
                        s.startframe = sx;
                        s.endframe   = ex;
                        selection.push_back(s);
                        }
                  }
            }
#endif
      return selection;
      }

//---------------------------------------------------------
//   modifySelection
//---------------------------------------------------------

void WaveView::modifySelection(int /*operation*/, unsigned /*startpos*/, unsigned /*stoppos*/, double /*paramA*/)
      {
#if 0
         song->startUndo();

         WaveSelectionList selection = getSelection(startpos, stoppos);
         for (iWaveSelection i = selection.begin(); i != selection.end(); i++) {
               WaveEventSelection w = *i;
               SndFileR& file         = w.file;
               unsigned sx            = w.startframe;
               unsigned ex            = w.endframe;
               unsigned file_channels = file.channels();

               QString tmpWavFile = QString::null;
               if (!getUniqueTmpfileName(tmpWavFile)) {
                     break;
                     }

               audio->msgIdle(true); // Not good with playback during operations
               SndFile tmpFile(tmpWavFile);
               tmpFile.setFormat(file.format(), file_channels, file.samplerate());
               if (tmpFile.openWrite()) {
                     audio->msgIdle(false);
                     printf("Could not open temporary file...\n");
                     break;
                     }

               //
               // Write out data that will be changed to temp file
               //
               unsigned tmpdatalen = ex - sx;
               off_t    tmpdataoffset = sx;
               float*   tmpdata[file_channels];

               for (unsigned i=0; i<file_channels; i++) {
                     tmpdata[i] = new float[tmpdatalen];
                     }

               file.seek(tmpdataoffset, 0);
               file.read(file_channels, tmpdata, tmpdatalen);
               file.close();
               tmpFile.write(file_channels, tmpdata, tmpdatalen);
               tmpFile.close();

               switch(operation)
               {
                     case MUTE:
                           muteSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case NORMALIZE:
                           normalizeSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_IN:
                           fadeInSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_OUT:
                           fadeOutSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case REVERSE:
                           reverseSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case GAIN:
                           applyGain(file_channels, tmpdata, tmpdatalen, paramA);
                           break;

                     case EDIT_EXTERNAL:
                           editExternal(file.format(), file.samplerate(), file_channels, tmpdata, tmpdatalen);
                           break;

                     default:
                           printf("Error: Default state reached in modifySelection\n");
                           break;

               }

               file.openWrite();
               file.seek(tmpdataoffset, 0);
               file.write(file_channels, tmpdata, tmpdatalen);
               file.update();
               file.close();
               file.openRead();

               for (unsigned i=0; i<file_channels; i++) {
                     delete[] tmpdata[i];
                     }

               // Undo handling
               song->cmdChangeWave(file.dirPath() + "/" + file.name(), tmpWavFile, sx, ex);
               audio->msgIdle(false); // Not good with playback during operations
               }
         song->endUndo(SC_CLIP_MODIFIED);
         update();
#endif
      }

//---------------------------------------------------------
//   muteSelection
//---------------------------------------------------------

void WaveView::muteSelection(unsigned /*channels*/, float** /*data*/, unsigned /*length*/)
      {
#if 0
      // Set everything to 0!
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = 0;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   normalizeSelection
//---------------------------------------------------------

void WaveView::normalizeSelection(unsigned /*channels*/, float** /*data*/, unsigned /*length*/)
      {
#if 0
      float loudest = 0.0;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  if (data[i][j]  > loudest)
                        loudest = data[i][j];
                  }
            }

      double scale = 0.99 / (double)loudest;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
#endif
      }

//---------------------------------------------------------
//   fadeInSelection
//---------------------------------------------------------

void WaveView::fadeInSelection(unsigned /*channels*/, float** /*data*/, unsigned /*length*/)
      {
#if 0
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) j / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
#endif
      }

//---------------------------------------------------------
//   fadeOutSelection
//---------------------------------------------------------

void WaveView::fadeOutSelection(unsigned /*channels*/, float** /*data*/, unsigned /*length*/)
      {
#if 0
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) (length - j) / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
#endif
      }

//---------------------------------------------------------
//   reverseSelection
//---------------------------------------------------------

void WaveView::reverseSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length/2; j++) {
                  float tmpl = data[i][j];
                  float tmpr = data[i][length - j];
                  data[i][j] = tmpr;
                  data[i][length - j] = tmpl;
                  }
            }
      }

//---------------------------------------------------------
//   applyGain
//---------------------------------------------------------

void WaveView::applyGain(unsigned channels, float** data, unsigned length, double gain)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * gain);
                  }
            }
      }

//---------------------------------------------------------
//   editExternal
//---------------------------------------------------------

void WaveView::editExternal(unsigned file_format, unsigned file_samplerate, unsigned file_channels, float** tmpdata, unsigned tmpdatalen)
      {
      // Create yet another tmp-file
      QString exttmpFileName;
      if (!getUniqueTmpfileName(exttmpFileName)) {
            printf("Could not create temp file - aborting...\n");
            return;
            }

      SndFile exttmpFile(exttmpFileName);
      exttmpFile.setFormat(file_format, file_channels, file_samplerate);
      if (exttmpFile.openWrite()) {
            printf("Could not open temporary file...\n");
            return;
            }
      // Write out change-data to this file:
      exttmpFile.write(file_channels, tmpdata, tmpdatalen);
      exttmpFile.close();

      // Forkaborkabork
      int pid = vfork();
      if (pid == 0) {
            if (execlp(config.externalWavEditor.toLatin1().data(), "", exttmpFileName.toLatin1().data(), NULL) == -1) {
                  perror("Failed to launch external editor");
                  // Get out of here
                  exit(-1);
                  }
                  //@!TODO: Handle unsuccessful attempts
            }
      else if (pid == -1) {
            perror("fork failed");
            }
      else {
            waitpid(pid, 0, 0);
            if (exttmpFile.openRead()) {
                  printf("Could not reopen temporary file again!\n");
                  }
            else {
                  // Re-read file again
                  exttmpFile.seek(0);
                  size_t sz = exttmpFile.read(file_channels, tmpdata, tmpdatalen);
                  if (sz != tmpdatalen) {
                        // File must have been shrunken - not good. Alert user.
                        QMessageBox::critical(this, tr("MusE - file size changed"),
                              tr("When editing in external editor - you should not change the filesize\nsince it must fit the selected region.\n\nMissing data is muted"));
                        for (unsigned i=0; i<file_channels; i++) {
                              for (unsigned j=sz; j<tmpdatalen; j++) {
                                    tmpdata[i][j] = 0;
                                    }
                              }
                        }
                  }
            QDir dir = exttmpFile.finfo()->absolutePath();
            dir.remove(exttmpFileName);
            dir.remove(exttmpFile.finfo()->baseName() + ".wca");
            }
      }

//---------------------------------------------------------
//   getUniqueTmpfileName
//---------------------------------------------------------

bool WaveView::getUniqueTmpfileName(QString& newFilename)
      {
      // Check if tmp-directory exists under project path
      QString tmpWavDir = song->absoluteProjectPath() + "/tmp_musewav"; //!@TODO: Don't hardcode like this
      QFileInfo tmpdirfi(tmpWavDir);
      if (!tmpdirfi.isDir()) {
            // Try to create a tmpdir
            QDir projdir(song->absoluteProjectPath());
            if (!projdir.mkdir("tmp_musewav")) {
                  printf("Could not create undo dir!\n");
                  return false;
                  }
            }


      tmpdirfi.setFile(tmpWavDir);

      if (!tmpdirfi.isWritable()) {
            printf("Temp directory is not writable - aborting\n");
            return false;
            }

      QDir tmpdir = tmpdirfi.dir();

      // Find a new filename
      for (int i=0; i<10000; i++) {
            QString filename = "muse_tmp";
            filename.append(QString::number(i));
            filename.append(".wav");

            if (!tmpdir.exists(tmpWavDir +"/" + filename)) {
                  newFilename = tmpWavDir + "/" + filename;
                  return true;
                  }
            }

      printf("Could not find a suitable tmpfilename (more than 10000 tmpfiles in tmpdir - clean up!\n");
      return false;
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void WaveView::range(AL::Pos& s, AL::Pos& e) const
      {
      s.setFrame(startFrame);
      e.setFrame(endFrame);
      }

