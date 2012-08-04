//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveview.cpp,v 1.10.2.16 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <limits.h>
#include <sys/wait.h>

#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFile>

#include "editgain.h"
#include "globals.h"
#include "wave.h"
#include "waveview.h"
#include "song.h"
#include "event.h"
#include "waveedit.h"
#include "audio.h"
#include "gconfig.h"
#include "fastlog.h"

namespace MusEGui {

bool modifyWarnedYet = false;
//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

WaveView::WaveView(MidiEditor* pr, QWidget* parent, int xscale, int yscale)
   : View(parent, xscale, 1)
      {
      editor = pr;
      setVirt(true);
      pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
      pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
      pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
      yScale = yscale;
      mode = NORMAL;
      selectionStart = 0;
      selectionStop  = 0;
      lastGainvalue = 100;

      setFocusPolicy(Qt::StrongFocus); // Tim.
      
      setMouseTracking(true);
      setBg(QColor(192, 208, 255));

      if (editor->parts()->empty()) {
            curPart = 0;
            curPartId = -1;
            }
      else {
            curPart   = (MusECore::WavePart*)(editor->parts()->begin()->second);
            curPartId = curPart->sn();
            }


      connect(MusEGlobal::song, SIGNAL(posChanged(int,unsigned,bool)), SLOT(setPos(int,unsigned,bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      songChanged(SC_SELECTION);
      }

//---------------------------------------------------------
//   setYScale
//---------------------------------------------------------

void WaveView::setYScale(int val)
      {
      yScale = val;
      redraw();
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveView::pdraw(QPainter& p, const QRect& rr)
      {
      int x1 = rr.x();
      int x2 = rr.right() + 1;
      if (x1 < 0)
            x1 = 0;
      if (x2 > width())
            x2 = width();
      int hh = height();
      int h  = hh/2;
      int y  = rr.y() + h;

      // Added by T356.
      int xScale = xmag;
      if (xScale < 0)
            xScale = -xScale;
      
      for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
            int channels = wp->track()->channels();
            int px = wp->frame();

            MusECore::EventList* el = wp->events();
            for (MusECore::iEvent e = el->begin(); e != el->end(); ++e) {
                  MusECore::Event event  = e->second;
                  if (event.empty())
                        continue;
                  MusECore::SndFileR f = event.sndFile();
                  if (f.isNull())
                        continue;
                  
                  unsigned peoffset = px + event.frame() - event.spos();
                  int sx, ex;
                  
                  sx = event.frame() + px + xScale/2;
                  ex = sx + event.lenFrame();
                  sx = sx / xScale - xpos;
                  ex = ex / xScale - xpos;

                  if (sx < x1)
                        sx = x1;
                  if (ex > x2)
                        ex = x2;

                  int pos = (xpos + sx) * xScale + event.spos() - event.frame() - px;
                  
                  //printf("pos=%d xpos=%d sx=%d ex=%d xScale=%d event.spos=%d event.frame=%d px=%d\n",
                  //      pos, xpos, sx, ex, xScale, event.spos(), event.frame(), px);
                  
                  h       = hh / (channels * 2);
                  int cc  = hh % (channels * 2) ? 0 : 1;

                  for (int i = sx; i < ex; i++) {
                        y  = rr.y() + h;
                        MusECore::SampleV sa[f.channels()];
                        f.read(sa, xScale, pos);
                        pos += xScale;
                        if (pos < event.spos())
                              continue;

                        int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
                        int selectionStopPos  = selectionStop  - peoffset;

                        for (int k = 0; k < channels; ++k) {
                              int kk = k % f.channels();
                              int peak = (sa[kk].peak * (h - 1)) / yScale;
                              int rms  = (sa[kk].rms  * (h - 1)) / yScale;
                              if (peak > h)
                                    peak = h;
                              if (rms > h)
                                    rms = h;
                              QColor peak_color = QColor(Qt::darkGray);
                              QColor rms_color  = QColor(Qt::black);
                              
                              // Changed by T356. Reduces (but not eliminates) drawing artifacts.
                              //if (pos > selectionStartPos && pos < selectionStopPos) {
                              if (pos > selectionStartPos && pos <= selectionStopPos) {
                                    
                                    peak_color = QColor(Qt::lightGray);
                                    rms_color  = QColor(Qt::white);
                                    // Draw inverted
                                    p.setPen(QColor(Qt::black));
                                    p.drawLine(i, y - h + cc, i, y + h - cc );
                                    }
                              p.setPen(peak_color);
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(rms_color);
                              p.drawLine(i, y - rms - cc, i, y + rms);
                              y  += 2 * h;
                              }
                        }
                  }
            }
      View::pdraw(p, rr);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveView::draw(QPainter& p, const QRect& r)
      {
      unsigned x = r.x() < 0 ? 0 : r.x();
      unsigned y = r.y() < 0 ? 0 : r.y();
      int w = r.width();
      int h = r.height();

      unsigned x2 = x + w;
      unsigned y2 = y + h;

      //
      //    draw marker & centerline
      //
      p.setPen(Qt::red);
      if (pos[0] >= x && pos[0] < x2) {
            p.drawLine(pos[0], y, pos[0], y2);
            }
      p.setPen(Qt::blue);
      if (pos[1] >= x && pos[1] < x2) {
            p.drawLine(pos[1], y, pos[1], y2);
            }
      if (pos[2] >= x && pos[2] < x2)
            p.drawLine(pos[2], y, pos[2], y2);

      int n = 1;
      if(curPart)
        n = curPart->track()->channels();
      
      int hn = h / n;
      int hh = hn / 2;
      for (int i = 0; i < n; ++i) {
            int h2     = hn * i;
            int center = hh + h2;
            p.setPen(QColor(i & i ? Qt::red : Qt::blue));
            p.drawLine(x, center, x2, center);
            p.setPen(QColor(Qt::black));
            p.drawLine(x, h2, x2, h2);
            }
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString WaveView::getCaption() const
      {
      if(curPart)
        return QString("Part ") + curPart->name();
      else  
        return QString("Part ");
        
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void WaveView::songChanged(int flags)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
    
      if (flags & ~SC_SELECTION) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in ecanvas.cpp)
            startSample  = INT_MAX;
            endSample    = 0;
            curPart      = 0;
            for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  MusECore::WavePart* part = (MusECore::WavePart*)(p->second);
                  if (part->sn() == curPartId)
                        curPart = part;
                  int ssample = part->frame();
                  int esample = ssample + part->lenFrame();
                  if (ssample < startSample) {
                        startSample = ssample;
                        //printf("startSample = %d\n", startSample);
                        }
                  if (esample > endSample) {
                        endSample = esample;
                        //printf("endSample = %d\n", endSample);
                        }
                  }
            }
      if (flags & SC_CLIP_MODIFIED) {
            redraw(); // Boring, but the only thing possible to do
            }
      if (flags & SC_TEMPO) {
            setPos(0, MusEGlobal::song->cpos(), false);
            setPos(1, MusEGlobal::song->lpos(), false);
            setPos(2, MusEGlobal::song->rpos(), false);
            }
      redraw();
      }

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void WaveView::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      val = MusEGlobal::tempomap.tick2frame(val);
      if (pos[idx] == val)
            return;
      int opos = mapx(pos[idx]);
      int npos = mapx(val);

      if (adjustScrollbar && idx == 0) {
            switch (MusEGlobal::song->follow()) {
                  case  MusECore::Song::NO:
                        break;
                  case MusECore::Song::JUMP:
                        if (npos >= width()) {
                              int ppos =  val - xorg - rmapxDev(width()/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - xorg - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
	    case MusECore::Song::CONTINUOUS:
                        if (npos > (width()*5)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*5/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()*3)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*3/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  }
            }

      int x;
      int w = 1;
      if (opos > npos) {
            w += opos - npos;
            x = npos;
            }
      else {
            w += npos - opos;
            x = opos;
            }
      pos[idx] = val;
      //redraw(QRect(x, 0, w, height()));
      redraw(QRect(x-1, 0, w+2, height()));    // p4.0.28 From Canvas::draw (is otherwise identical). Fix for corruption.
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void WaveView::viewMousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      unsigned x = event->x();

      switch (button) {
            case Qt::LeftButton:
                  if (mode == NORMAL) {
                        // redraw and reset:
                        if (selectionStart != selectionStop) {
                              selectionStart = selectionStop = 0;
                              redraw();
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
      }

#define WHEEL_STEPSIZE 40
#define WHEEL_DELTA   120
//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void WaveView::wheelEvent(QWheelEvent* ev)
{
  int keyState = ev->modifiers();

  bool shift      = keyState & Qt::ShiftModifier;
  bool ctrl       = keyState & Qt::ControlModifier;

  if (shift) { // scroll vertically
      int delta       = -ev->delta() / WHEEL_DELTA;
      int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));


      if (xpixelscale <= 0)
            xpixelscale = 1;

      int scrollstep = WHEEL_STEPSIZE * (delta);
      ///if (ev->state() == Qt::ShiftModifier)
  //      if (((QInputEvent*)ev)->modifiers() == Qt::ShiftModifier)
      scrollstep = scrollstep / 10;

      int newXpos = xpos + xpixelscale * scrollstep;

      if (newXpos < 0)
            newXpos = 0;

      //setYPos(newYpos);
      emit horizontalScroll((unsigned)newXpos);


  } else if (ctrl) {  // zoom horizontally
    if (ev->delta()>0)
      emit horizontalZoomIn();
    else
      emit horizontalZoomOut();

  } else { // scroll horizontally
      emit mouseWheelMoved(ev->delta() / 10);
  }

}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------
void WaveView::viewMouseReleaseEvent(QMouseEvent* /*event*/)
      {
      button = Qt::NoButton;

      if (mode == DRAG) {
            mode = NORMAL;
            //printf("selectionStart=%d selectionStop=%d\n", selectionStart, selectionStop);
            }
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void WaveView::viewMouseMoveEvent(QMouseEvent* event)
      {
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
                  if ((MusEGlobal::config.rangeMarkerWithoutMMB) && (event->modifiers() & Qt::ControlModifier))
                      i = 1;
                  else
                      i = 2;
                  break;
            default:
                  return;
            }
      MusECore::Pos p(MusEGlobal::tempomap.frame2tick(x), true);
      MusEGlobal::song->setPos(i, p);
      }

//---------------------------------------------------------
//   range
//    returns range in samples
//---------------------------------------------------------

void WaveView::range(int* s, int *e)
      {
      
      MusECore::PartList* lst = editor->parts();
      if(lst->empty())
      {
        *s = 0;
        *e = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->len());
        return;  
      }
      int ps = MusEGlobal::song->len(), pe = 0;
      int tps, tpe;
      for(MusECore::iPart ip = lst->begin(); ip != lst->end(); ++ip) 
      {
        tps = ip->second->tick();
        if(tps < ps)
          ps = tps;
        tpe = tps + ip->second->lenTick();
        if(tpe > pe)
          pe = tpe;
      }
      *s = MusEGlobal::tempomap.tick2frame(ps);
      *e = MusEGlobal::tempomap.tick2frame(pe);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------
void WaveView::cmd(int n)
      {
      int modifyoperation = -1;
      double paramA = 0.0;

      switch(n) {
            case WaveEdit::CMD_SELECT_ALL:
            if (!editor->parts()->empty()) {
                  MusECore::iPart iBeg = editor->parts()->begin();
                  MusECore::iPart iEnd = editor->parts()->end();
                  iEnd--;
                  MusECore::WavePart* beg = (MusECore::WavePart*) iBeg->second;
                  MusECore::WavePart* end = (MusECore::WavePart*) iEnd->second;
                  selectionStart = beg->frame();
                  selectionStop  = end->frame() + end->lenFrame();
                  redraw();
                  }
                  break;

            case WaveEdit::CMD_EDIT_EXTERNAL:
                  modifyoperation = EDIT_EXTERNAL;
                  break;

            case WaveEdit::CMD_SELECT_NONE:
                  selectionStart = selectionStop = 0;
                  redraw();
                  break;
            case WaveEdit::CMD_EDIT_COPY:
                  modifyoperation = COPY;
                  break;
            case WaveEdit::CMD_EDIT_CUT:
                  modifyoperation = CUT;
                  break;
            case WaveEdit::CMD_EDIT_PASTE:
                  modifyoperation = PASTE;
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
                  EditGain* editGain = new EditGain(this, lastGainvalue);
                  if (editGain->exec() == QDialog::Accepted) {
                        lastGainvalue = editGain->getGain();
                        modifyoperation = GAIN;
                        paramA = (double)lastGainvalue / 100.0;
                        }
                  delete editGain;
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
            if (selectionStart == selectionStop && modifyoperation!=PASTE) {
                  printf("No selection. Ignoring\n"); //@!TODO: Disable menu options when no selection
                  QMessageBox::information(this, 
                     QString("MusE"),
                     QWidget::tr("No selection. Ignoring"));

                  return;
                  }
            
            //if(!modifyWarnedYet)
            //{
            //  modifyWarnedYet = true;
            //  if(QMessageBox::warning(this, QString("Muse"),
            //     tr("Warning! Muse currently operates directly on the sound file.\n"
            //        "Undo is supported, but NOT after exit, WITH OR WITHOUT A SAVE!\n"
            //        "If you are stuck, try deleting the associated .wca file and reloading."), tr("&Ok"), tr("&Cancel"),
            //     QString::null, 0, 1 ) != 0)
            //   return;
            //}
            modifySelection(modifyoperation, selectionStart, selectionStop, paramA);
            }
      }


//---------------------------------------------------------
//   getSelection
//---------------------------------------------------------
MusECore::WaveSelectionList WaveView::getSelection(unsigned startpos, unsigned stoppos)
      {
      MusECore::WaveSelectionList selection;

      for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
            unsigned part_offset = wp->frame();
            
            MusECore::EventList* el = wp->events();
            //printf("eventlist length=%d\n",el->size());

            for (MusECore::iEvent e = el->begin(); e != el->end(); ++e) {
                  MusECore::Event event  = e->second;
                  if (event.empty())
                        continue;
                  MusECore::SndFileR file = event.sndFile();
                  if (file.isNull())
                        continue;
                  
                  unsigned event_offset = event.frame() + part_offset;
                  unsigned event_startpos  = event.spos();
                  unsigned event_length = event.lenFrame() + event.spos();
                  unsigned event_end    = event_offset + event_length;
                  //printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n", startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);

                  if (!(event_end <= startpos || event_offset > stoppos)) {
                        int tmp_sx = startpos - event_offset + event_startpos;
                        int tmp_ex = stoppos  - event_offset + event_startpos;
                        unsigned sx;
                        unsigned ex;

                        tmp_sx < (int)event_startpos ? sx = event_startpos : sx = tmp_sx;
                        tmp_ex > (int)event_length   ? ex = event_length   : ex = tmp_ex;

                        //printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLatin1().constData());
                        MusECore::WaveEventSelection s;
                        s.file = file;
                        s.startframe = sx;
                        s.endframe   = ex+1;
                        //printf("sx=%d ex=%d\n",sx,ex);
                        selection.push_back(s);
                        }
                  }
            }

            return selection;
      }

//---------------------------------------------------------
//   modifySelection
//---------------------------------------------------------
void WaveView::modifySelection(int operation, unsigned startpos, unsigned stoppos, double paramA)
      {
         MusEGlobal::song->startUndo();

         if (operation == PASTE) {
           // we need to redefine startpos and stoppos
           if (copiedPart =="")
             return;
           MusECore::SndFile pasteFile(copiedPart);
           pasteFile.openRead();
           startpos = pos[0];
           stoppos = startpos+ pasteFile.samples(); // possibly this is wrong if there are tempo changes
           pasteFile.close();
           pos[0]=stoppos;
         }

	 MusECore::WaveSelectionList selection = getSelection(startpos, stoppos);
         for (MusECore::iWaveSelection i = selection.begin(); i != selection.end(); i++) {
               MusECore::WaveEventSelection w = *i;
               MusECore::SndFileR& file         = w.file;
               unsigned sx            = w.startframe;
               unsigned ex            = w.endframe;
               unsigned file_channels = file.channels();

               QString tmpWavFile = QString::null;
               if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", tmpWavFile)) {
                     break;
                     }

               MusEGlobal::audio->msgIdle(true); // Not good with playback during operations
               MusECore::SndFile tmpFile(tmpWavFile);
               tmpFile.setFormat(file.format(), file_channels, file.samplerate());
               if (tmpFile.openWrite()) {
                     MusEGlobal::audio->msgIdle(false);
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
               file.readWithHeap(file_channels, tmpdata, tmpdatalen);
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
                     case CUT:
                           copySelection(file_channels, tmpdata, tmpdatalen, true, file.format(), file.samplerate());
                           break;
                     case COPY:
                           copySelection(file_channels, tmpdata, tmpdatalen, false, file.format(), file.samplerate());
                           break;
                     case PASTE:
                           {
                           MusECore::SndFile pasteFile(copiedPart);
                           pasteFile.openRead();
                           pasteFile.seek(tmpdataoffset, 0);
                           pasteFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
                           }
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
               MusEGlobal::song->cmdChangeWave(file.dirPath() + "/" + file.name(), tmpWavFile, sx, ex);
               MusEGlobal::audio->msgIdle(false); // Not good with playback during operations
               }
         MusEGlobal::song->endUndo(SC_CLIP_MODIFIED);
         redraw();
      }

//---------------------------------------------------------
//   copySelection
//---------------------------------------------------------
void WaveView::copySelection(unsigned file_channels, float** tmpdata, unsigned length, bool blankData, unsigned format, unsigned sampleRate)
{
      if (copiedPart!="") {
        QFile::remove(copiedPart);
      }
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", copiedPart)) {
            return;
            }

      MusECore::SndFile tmpFile(copiedPart);
      tmpFile.setFormat(format, file_channels, sampleRate);
      tmpFile.openWrite();
      tmpFile.write(file_channels, tmpdata, length);
      tmpFile.close();

      if (blankData) {
        // Set everything to 0!
        for (unsigned i=0; i<file_channels; i++) {
              for (unsigned j=0; j<length; j++) {
                    tmpdata[i][j] = 0;
                    }
              }
        }
}

//---------------------------------------------------------
//   muteSelection
//---------------------------------------------------------
void WaveView::muteSelection(unsigned channels, float** data, unsigned length)
      {
      // Set everything to 0!
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   normalizeSelection
//---------------------------------------------------------
void WaveView::normalizeSelection(unsigned channels, float** data, unsigned length)
      {
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
      }

//---------------------------------------------------------
//   fadeInSelection
//---------------------------------------------------------
void WaveView::fadeInSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) j / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   fadeOutSelection
//---------------------------------------------------------
void WaveView::fadeOutSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) (length - j) / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   reverseSelection
//---------------------------------------------------------
void WaveView::reverseSelection(unsigned channels, float** data, unsigned length)
      {
      if(length <= 1)    
        return;
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length/2; j++) {
                  float tmpl = data[i][j];
                  float tmpr = data[i][length - j - 1];
                  data[i][j] = tmpr;
                  data[i][length - j - 1] = tmpl;
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
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", exttmpFileName)) {
            printf("Could not create temp file - aborting...\n");
            return;
            }

      MusECore::SndFile exttmpFile(exttmpFileName);
      exttmpFile.setFormat(file_format, file_channels, file_samplerate);
      if (exttmpFile.openWrite()) {
            printf("Could not open temporary file...\n");
            return;
            }
      // Write out change-data to this file:
      exttmpFile.write(file_channels, tmpdata, tmpdatalen);
      exttmpFile.close();

      // Forkaborkabork
      int pid = fork();
      if (pid == 0) {
            if (execlp(MusEGlobal::config.externalWavEditor.toLatin1().constData(), MusEGlobal::config.externalWavEditor.toLatin1().constData(), exttmpFileName.toLatin1().constData(), NULL) == -1) {
                  perror("Failed to launch external editor");
                  // Get out of here
                  
                   
                  // cannot report error through gui, we are in another fork!
                  //@!TODO: Handle unsuccessful attempts
                  exit(99);
                  }
            exit(0);
            }
      else if (pid == -1) {
            perror("fork failed");
            }
      else {
            int status;
            waitpid(pid, &status, 0);
            //printf ("status=%d\n",status);
            if( WEXITSTATUS(status) != 0 ){
                   QMessageBox::warning(this, tr("MusE - external editor failed"),
                         tr("MusE was unable to launch the external editor\ncheck if the editor setting in:\n"
                         "Global Settings->Audio:External Waveditor\nis set to a valid editor."));
            }
            
            if (exttmpFile.openRead()) {
                printf("Could not reopen temporary file!\n");
                }
            else {
                // Re-read file again
                exttmpFile.seek(0, 0);
                size_t sz = exttmpFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
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
            QDir dir = exttmpFile.dirPath();
            dir.remove(exttmpFileName);
            dir.remove(exttmpFile.basename() + ".wca");
            }
      }

} // namespace MusECore
