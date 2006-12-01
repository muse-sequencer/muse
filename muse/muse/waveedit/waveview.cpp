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
#include "widgets/simplebutton.h"
#include "utils.h"

static const int partLabelHeight = 13;

//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

WaveView::WaveView(WaveEdit* pr)
   : TimeCanvas(TIME_CANVAS_WAVEEDIT)
      {
      setMarkerList(song->marker());
      curSplitter    = -1;
      dragSplitter   = false;
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
//   mousePress
//---------------------------------------------------------

void WaveView::mousePress(QMouseEvent* me)
      {
      QPoint pos(me->pos());

      if (rCanvasA.contains(pos)) {
//            mousePressCanvasA(me);
            return;
            }
      if (curSplitter != -1) {
            dragSplitter = true;
            splitterY = pos.y();
            return;
            }

      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mousePress(pos - r.topLeft(), me->button(), me->modifiers());
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void WaveView::mouseRelease(QMouseEvent* me)
      {
      if (dragSplitter) {
            dragSplitter = false;
            return;
            }
      QPoint pos(me->pos());
      if (rCanvasA.contains(pos)) {
            // mouseReleaseCanvasA(me);
            return;
            }
      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mouseRelease();
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void WaveView::mouseMove(QPoint pos)
      {
      if (dragSplitter) {
            int deltaY = pos.y() - splitterY;

            iCtrlEdit i = ctrlEditList.begin();
            int y = 0;
            if (curSplitter > 0) {
                  int k = 0;
                  CtrlEdit* c = 0;
                  for (; i != ctrlEditList.end(); ++i, ++k) {
                        c = *i;
                        y += c->height();
                        if ((k+1) == curSplitter)
                                    break;
                        }
                  if (i == ctrlEditList.end()) {
                        printf("unexpected edit list end, curSplitter %d\n", curSplitter);
                        return;
                        }
                  if (c->height() + deltaY < splitWidth)
                        deltaY = splitWidth - c->height();
                  ++i;
                  int rest = 0;
                  for (iCtrlEdit ii = i; ii != ctrlEditList.end(); ++ii)
                        rest += (*ii)->cheight();
                  if (rest < deltaY)
                        deltaY = rest;
                  c->setHeight(c->height() + deltaY);
                  layoutPanelB(c);
                  y += deltaY;
                  }
            //
            //    layout rest, add deltaY vertical
            //
            int rest = 0;
            for (iCtrlEdit ii = i; ii != ctrlEditList.end(); ++ii) {
                  CtrlEdit* c = *ii;
                  rest += c->cheight();
                  }
            if (rest < deltaY)
                        deltaY = rest;
            rest = deltaY;
            for (; i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  int d = c->cheight();
                  if (d > deltaY)
                              d = deltaY;
                  c->setHeight(c->height() - d);
                  c->y = y;
                  layoutPanelB(c);
                  y += c->height();
                  deltaY -= d;
                  if (deltaY == 0)
                              break;
                  }
            if (i != ctrlEditList.end())
                  ++i;
            for (; i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  c->y = y;
                  y += c->height();
                  }
            if (curSplitter == 0)
                        resizeController(ctrlHeight - rest);
            else
                        widget()->update(rPanelB | rCanvasB);
            splitterY = pos.y();
            updatePartControllerList();
            return;
            }
      if (rCanvasA.contains(pos)) {
            // mouseMoveCanvasA(pos - rCanvasA.topLeft());
            return;
            }
      if (button == 0) {
            if (rPanelB.contains(pos) || rCanvasB.contains(pos)) {
                  int y = pos.y() - rPanelB.y();
                  int k = 0;
                  for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
                        CtrlEdit* c = *i;
                        if (y >= c->y && y < (c->y + splitWidth)) {
                              curSplitter = k;
                              setCursor();
                              return;
                              }
                        int ypos = y - c->y - splitWidth;
                        if (ypos >= 0)
                              emit yChanged(c->pixel2val(ypos));
                        }
                  }
            if (curSplitter != -1) {
                  curSplitter = -1;
                  setCursor();
                  }
            return;
            }
      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mouseMove(pos - r.topLeft());
                        break;
                        }
                  }
            }

      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveView::cmd(const QString&)
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

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void WaveView::layout()
      {
      int n = ctrlEditList.size();
      if (n == 0)
            return;
      if (ctrlHeight == 0) {
            int wh = widget()->height();
            resizeController(wh < 120 ? wh / 2 : 100);
            }
      // check, if layout is ok already; this happens after
      // song load
      int h = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            h += c->height();
            }
      if (h == ctrlHeight) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
                  layoutPanelB(*i);
            return;
            }
      int y = 0;
      int sch = ctrlHeight / n;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            c->y = y;
            c->setHeight(sch);
            layoutPanelB(c);
            y += sch;
            }      
      }

//---------------------------------------------------------
//   layout1
//---------------------------------------------------------

void WaveView::layout1()
      {
      int n = ctrlEditList.size();
      if (n == 0)
            return;
      int y = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            c->y = y;
            y += c->height();
            }
      resizeController(y);      
      }

//---------------------------------------------------------
//   layoutPanelB
//---------------------------------------------------------

void WaveView::layoutPanelB(CtrlEdit* c)
      {
      int y = c->y;
      int h = c->height();
      int bx = rPanelB.x() + rPanelB.width() - 23;
      int by = rPanelB.y() + y + h - 19;
      c->minus->setGeometry(bx, by, 18, 18);
      bx = rPanelB.x() + 1;
      by = rPanelB.y() + y + 5;
      c->sel->setGeometry(bx, by, rPanelB.width() - 5, 18);
      }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void WaveView::addController()
      {
      int n = ctrlEditList.size();
      CtrlEdit* ce = new CtrlEdit(widget(), this, curPart->track());
      ce->setHeight(50);
      ctrlEditList.push_back(ce);

      ce->minus->defaultAction()->setData(n);
      connect(ce->minus, SIGNAL(triggered(QAction*)), SLOT(removeController(QAction*)));
      ce->minus->show();
      ce->sel->show();

      layout();
      widget()->update();
      updatePartControllerList();
      }

void WaveView::addController(int id, int h)
      {
      ctrlHeight += h;
      int n = ctrlEditList.size();
      
      CtrlEdit* ce = new CtrlEdit(widget(), this, curPart->track());
      ce->setHeight(h);
      ce->setCtrl(id);
      ctrlEditList.push_back(ce);

      ce->minus->defaultAction()->setData(n);
      connect(ce->minus, SIGNAL(triggered(QAction*)), SLOT(removeController(QAction*)));
      }

//---------------------------------------------------------
//   removeController
//---------------------------------------------------------

void WaveView::removeController(QAction* a)
      {
      int id = a->data().toInt();

      int k = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
            if (k == id) {
                  CtrlEdit* c = *i;
                  delete c;
                  ctrlEditList.erase(i);
                  break;
                  }
             }
      k = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
            CtrlEdit* c = *i;
            c->minus->defaultAction()->setData(k);
            }

      if (ctrlEditList.empty())
            resizeController(0);
      else
            layout();
      widget()->update();
      updatePartControllerList();
      }

//---------------------------------------------------------
//   updatePartControllerList
//---------------------------------------------------------

void WaveView::updatePartControllerList()
      {
      if (curPart == 0)
            return;
      CtrlCanvasList* cl = curPart->getCtrlCanvasList();
      cl->clear();
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlCanvas cc;
            cc.ctrlId = (*i)->ctrlId;
            cc.height = (*i)->height();
            cl->push_back(cc);
            }      
      }

//---------------------------------------------------------
//   paintControllerCanvas
//    r(0, 0) is PanelB topLeft()
//---------------------------------------------------------

void WaveView::paintControllerCanvas(QPainter& p, QRect r)
      {
      int x1 = r.x();
      int x2 = x1 + r.width();

      int xx2 = rCanvasB.width();
      if (xx2 >= x2)
                  x2 = xx2 - 2;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            int         y = c->y;
            paintHLine(p, x1, x2, y);
            p.setPen(lineColor[0]);
            p.drawLine(xx2-1, 1, xx2-1, splitWidth-2);

            QRect  rc(0, y + splitWidth, rCanvasB.width(), c->cheight());
            QPoint pt(rc.topLeft());
            rc &= r;
            if (!rc.isEmpty()) {
                  p.translate(pt);
                  c->paint(p, rc.translated(-pt));
                  p.translate(-pt);
                  }
            }
      }

//---------------------------------------------------------
//   paintControllerPanel
//    panelB
//---------------------------------------------------------

void WaveView::paintControllerPanel(QPainter& p, QRect r)
      {
      p.fillRect(r, QColor(0xe0, 0xe0, 0xe0));
      int x1 = r.x();
      int x2 = x1 + r.width();

      paintVLine(p, r.y() + splitWidth, r.y() + r.height(),
         rPanelB.x() + rPanelB.width());

      if (x1 == 0)
                  x1 = 1;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            paintHLine(p, x1, x2, c->y);
            p.setPen(lineColor[0]);
            p.drawLine(0, 1, 0, splitWidth-2);
            }
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void WaveView::setCursor()
      {
      if (curSplitter != -1) {
            widget()->setCursor(Qt::SplitVCursor);
            return;
            }
      TimeCanvas::setCursor();
      }

//---------------------------------------------------------
//   enterB
//---------------------------------------------------------

void WaveView::enterB()
      {
      if ((button == 0) && curSplitter != -1) {
            curSplitter = -1;
            setCursor();
            }
      }

//---------------------------------------------------------
//   leaveB
//---------------------------------------------------------

void WaveView::leaveB()
      {
      if ((button == 0) && (curSplitter != -1)) {
            curSplitter = -1;
            setCursor();
            }
      }


