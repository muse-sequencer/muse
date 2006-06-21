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

#include "canvas.h"
#include "al/al.h"
#include "al/sig.h"
#include "gconfig.h"
#include "song.h"
#include "icons.h"
#include "audio.h"
#include "partdrag.h"
#include "muse.h"
#include "midictrl.h"
#include "tlswidget.h"
#include "part.h"
#include "gui.h"

#include <samplerate.h>

static const int partLabelHeight = 13;
static const int handleWidth     = 5;
static const int partBorderWidth = 2;

enum { HIT_NOTHING, HIT_TRACK, HIT_PART, HIT_SUBTRACK };

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

PartCanvas::PartCanvas()
   : TimeCanvas(TIME_CANVAS)
      {
      state = S_NORMAL;
      _drawBackground = true;
      lselected  = -1;
      starty     = -1;
      setMarkerList(song->marker());
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

void PartCanvas::drawWavePart(QPainter& p, Part* wp, int y0, int th, int from, int to)
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
//   paint
//---------------------------------------------------------

void PartCanvas::paint(QPainter& p, QRect r)
      {
// printf("canvas paint %d %d %d %d\n", r.x(), r.y(), r.width(), r.height());
      QFont f = font();
      f.setPointSize(8);
      p.setFont(f);

      int from = r.x();
      int to   = from + r.width();

      TrackList* tl = song->tracks();
      ArrangerTrack* at = 0;
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            Track* t = *i;
            at = &(t->arrangerTrack);
            if (at->tw == 0)
                  continue;

            int y = at->tw->y() - splitWidth/2;
            int h = at->tw->height() - 1;

            PartList* pl = t->parts();
            for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  Part* part = ip->second;
                  int x1  = pos2pix(*part);
                  int x2  = pos2pix(part->end());
                  int len = x2 - x1;

                  if (x2 <= from)
                        continue;
                  if (x1 > to)
                        break;

                  QRect pr(x1, y, len, h - partBorderWidth);

                  if (part->mute()) {
                        p.setPen(QPen(Qt::red, partBorderWidth));
                        p.setBrush(Qt::gray);
                        }
                  else if (part->selected()) {
                        p.setPen(QPen(config.partColors[part->colorIndex()], partBorderWidth));
                        p.setBrush(config.selectPartBg);
                        }
                  else {
                        bool clone = part->events()->arefCount() > 1;
                        p.setPen(QPen(Qt::black, partBorderWidth, clone ? Qt::DashLine : Qt::SolidLine));
                        p.setBrush(config.partColors[part->colorIndex()]);
                        }

                  //
                  // we want to draw the rectangle without transformation
                  // to get equal border width horizontal and vertical
                  //
                  QRect rr(p.matrix().mapRect(pr).adjusted(1, 0, -1, 0));
                  p.save();
                  p.resetMatrix();
                  p.drawRect(rr);
                  p.restore();

                  int xx1 = x1;
                  if (xx1 < from)
                        xx1 = from;
                  int xx2 = x2;
                  if (xx2 > to)
                        xx2 = to;
                  if (t->isMidiTrack())
                        drawMidiPart(p, part, y, h, xx1, xx2);
                  else if (t->type() == Track::WAVE)
                        drawWavePart(p, part, y, h, xx1, xx2);
                  int yy = y + h - partLabelHeight;
                  p.drawText(x1 + 3, yy, len - 6,
                     partLabelHeight-1, Qt::AlignVCenter | Qt::AlignLeft,
                     part->name());
                  }
            p.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine));
            if (i != tl->begin())
	            p.drawLine(from, y-2, to, y-2);
            for (iArrangerTrack i = t->subtracks.begin(); i != t->subtracks.end(); ++i) {
                  at = *i;
                  if (at->tw == 0)
                        continue;
                  TLSWidget* tls = (TLSWidget*)(at->tw);
                  int y = tls->y();
                  p.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine));
                  p.drawLine(from, y-2, to, y-2);
                  QPoint off(0, y);
                  p.translate(off);
                  tls->paint(p, r);
                  p.translate(-off);
                  }
            }
      if (at && at->tw) {
            // draw last line
            int y = at->tw->y() + at->tw->height() - 2;
            p.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine));
            p.drawLine(from, y-2, to, y-2);
            }
      if (state == S_DRAG4 || state == S_DRAG1 || state == S_DRAG2 || state == S_DRAG5) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(Qt::red), partBorderWidth));
            p.drawRect(drag);
            }
      }

//---------------------------------------------------------
//   drawMidiSubPart
//---------------------------------------------------------

void PartCanvas::drawMidiPart(QPainter& p, Part* mp, int y, int th, int from, int to)
      {
      p.setPen(Qt::black);

      EventList* events = mp->events();
      // iEvent ito(events->lower_bound(to));

      iEvent ito(events->end());
//      int pos = pos2pix(*mp);

      if (config.canvasShowPartType & 2) {      // show events
            for (iEvent i = events->lower_bound(from); i != ito; ++i) {
                  EventType type = i->second.type();
                  if (
                     ((config.canvasShowPartEvent & 1) && (type == Note))
                     || ((config.canvasShowPartEvent & 2) && (type == PAfter))
                     || ((config.canvasShowPartEvent & 4) && (type == Controller))
                     || ((config.canvasShowPartEvent &16) && (type == CAfter))
                     || ((config.canvasShowPartEvent &64) && (type == Sysex || type == Meta))
                     ) {
                        int t = pos2pix(*mp + i->second.pos());
                        p.drawLine(t, y + 1, t, y + th - 2);
                        }
                  }
            }

      else {      // show Cakewalk Style
            for (iEvent i = events->begin(); i != ito; ++i) {
                  if (i->second.type() != Note)
                        continue;
                  int x1 = pos2pix(*mp + i->second.pos());
                  int x2 = pos2pix(*mp + i->second.end());

                  if (x1 > to)
                        break;
                  if (x2 < from)
                        continue;

                  if (x2 > to)       // clip to drawing area
                        x2 = to;
                  if (x1 < from)
                        x1 = from;
                  int pitch = i->second.pitch();
                  int yy     = y + th - (pitch * th / 127);
                  p.drawLine(x1, yy, x2, yy);
                  }
            }
      }

//---------------------------------------------------------
//   searchPart
//---------------------------------------------------------

int PartCanvas::searchPart(const QPoint& pp)
      {
      Pos tp(pix2pos(pp.x()));
      QPoint p(tp.tick(), pp.y() + wpos.y());

// printf("searchPart %d %d\n", p.x(), p.y());
      track = 0;
      part  = 0;
      at    = 0;
      int yp = p.y();
      if (yp < 0)
            return HIT_NOTHING;

      TrackList* tl = song->tracks();
      iTrack i;
      int y1, y2;
      for (i = tl->begin(); i != tl->end(); ++i) {
            track = *i;
            QWidget* tw = track->arrangerTrack.tw;
            if (tw == 0) {
                  printf(" invisible Track\n");
                  continue;
                  }
            y1 = tw->y();
            y2 = y1 + tw->height();
            if (yp >= y1 && yp < y2) {
// printf("  track <%s> %d - %d\n", track->name().toLatin1().data(), y1, y2);
                  break;
                  }
            for (iArrangerTrack i = track->subtracks.begin(); i != track->subtracks.end(); ++i) {
                  at = *i;
                  if (at->tw == 0) {
                        printf("----empty subtrack?\n");
                        break;
                        }
                  y1 = at->tw->y();
                  y2 = y1 + at->tw->height();
                  if (yp >= y1 && yp < y2) {
                        return HIT_SUBTRACK;
                        }
                  }
            }
      if (i == tl->end()) {
            track = 0;
            at    = 0;
            return HIT_NOTHING;
            }

      unsigned x = p.x();
      PartList* pl = track->parts();
      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            part    = ip->second;
            if (x >= part->tick() && x < part->endTick()) {
                  ppos  = mapx(part->tick());
                  psize = rmapx(part->lenTick());
// printf("  part\n");
                  return HIT_PART;
                  }
            }
      part = 0;
      return HIT_TRACK;
      }

//---------------------------------------------------------
//   contextMenu
//---------------------------------------------------------

void PartCanvas::contextMenu(const QPoint& pos)
      {
      QMenu* pop = new QMenu(widget());
      QAction* a;
      if (part) {
            a = pop->addAction(*editcutIconSet, tr("C&ut"));
            a->setData(4);
            a->setShortcut(Qt::CTRL + Qt::Key_X);

            a = pop->addAction(*editcopyIconSet, tr("&Copy"));
            a->setData(5);
            a->setShortcut(Qt::CTRL + Qt::Key_C);

            pop->addSeparator();

            a = pop->addAction(tr("rename"));
            a->setData(0);

            QMenu* cp = pop->addMenu(tr("color"));

            // part color selection
            for (int i = 0; i < NUM_PARTCOLORS; ++i) {
                  a = cp->addAction(partColorNames[i]);
                  a->setData(20 + i);
                  QPixmap pm(20, 20);
                  QPainter p(&pm);
                  p.fillRect(0, 0, 20, 20, config.partColors[i]);
                  a->setIcon(QIcon(pm));
                  }
            a = pop->addAction(*deleteIcon, tr("delete"));
            a->setData(1);
            a = pop->addAction(*cutIcon, tr("split"));
            a->setData(2);
            a = pop->addAction(*glueIcon, tr("glue"));
            a->setData(3);
            a = pop->addAction(tr("de-clone"));
            a->setData(15);
            a->setEnabled(part->events()->arefCount() > 1);
            if (track->type() == Track::MIDI) {
                  a = pop->addAction(tr("AutoFill..."));
                  a->setData(16);
                  }
            pop->addSeparator();
      	if (track->type() == Track::MIDI) {
                  MidiTrack* track = (MidiTrack*)part->track();
                  MidiChannel* mc = track->channel();
                  if (mc && mc->useDrumMap()) {
                  	a = pop->addAction(*edit_drummsIcon, tr("drums"));
                        a->setData(13);
                        }
                  else {
                        a = pop->addAction(*pianoIconSet, tr("pianoroll"));
                        a->setData(10);
                        }
		  a = pop->addAction(*edit_listIcon, tr("soundtracker"));
                  a = pop->addAction(*edit_listIcon, tr("list"));
                  a->setData(12);
                  }
            else {
			a = pop->addAction(*waveIcon, tr("wave edit"));
                  a->setData(14);
                  }

            a = pop->exec(mapToGlobal(pos));
            if (a) {
                  int n = a->data().toInt();
                  switch (n) {
                        case 0:
                              renamePart(part);
                              break;
                        case 1:
                              audio->msgRemovePart(part, true);
                              track->partListChanged();
                              break;
                        case 2:
                              splitPart(part, startDrag);
                              break;
                        case 3:
                              song->cmdGluePart(part);
                              break;
                        case 4:
                              cutPart(part);
                              break;
                        case 5:
                              copyPart(part);
                              break;
                        case 10:    // pianoroll edit
                              emit startEditor(part, 0);
                              break;
                        case 12:    // list edit
                              emit startEditor(part, 1);
                              break;
                        case 13:    // drum edit
                              emit startEditor(part, 3);
                              break;
                        case 14:
                              emit startEditor(part, 4);
                              break;
                        case 15:
                              declonePart(part);
                              break;
                        case 16:
                        	// auto fill: ask for loop length
                        	{
                              bool ok;
                              int ticksM = AL::sigmap.ticksMeasure(part->tick());
                              int n = QInputDialog::getInteger(this,
                        	   tr("MusE: Get auto fill loop len"),
                        	   tr("Measures: "),
                        	   part->fillLen() / ticksM,
                        	   0, 16, 1, &ok);
                        	if (ok) {
                                    part->setFillLen(n * ticksM);
                                    }
                              }
                        	break;
                        case 20 ... NUM_PARTCOLORS+20:
                              part->setColorIndex(n - 20);
                              widget()->update();
                              break;
                        case -1:
                              break;
                        default:
                              printf("unknown action %d\n", n);
                              break;
                        }
                  }
            }
      else {
            for (int i = 0; i < TOOLS; ++i) {
                  if ((arrangerTools & (1 << i))==0)
                        continue;
                  a = pop->addAction(**toolList[i].icon, tr(toolList[i].tip));
                  int id = 1 << i;
                  a->setData(id);
                  a->setCheckable(true);
                  if (id == (int)_tool)
                        a->setChecked(true);
                  }
            a = pop->exec(mapToGlobal(pos));
            if (a) {
                  int n = a->data().toInt();
                  muse->setTool(n);
                  }
            }
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PartCanvas::mousePress(QMouseEvent* me)
      {
      QPoint pos(me->pos().x(), me->pos().y() - rulerHeight);
      startDrag = pos;
      int hit   = searchPart(startDrag);

      if (hit == HIT_SUBTRACK) {
            TLSWidget* w = (TLSWidget*)(at->tw);
            int y = pos.y() - w->y() + 2;
            w->mousePress(QPoint(pos.x(), y), button);
            state = S_SUBTRACK;
            return;
            }

      if (button & Qt::RightButton) {
            contextMenu(pos);
            return;
            }

      QRect r1,r2;
      QPoint pos2;
      int xpos = 0, y = 0, len = 0, h = 0;

      if (hit == HIT_PART) {
  		h    = track->arrangerTrack.tw->height();
      	len  = rmapx(part->lenTick());
      	y    = track->arrangerTrack.tw->y() - splitWidth/2;
		xpos = mapx(part->tick());

		r1 = QRect(xpos, y, handleWidth, h);
	      r2 = QRect(xpos + len - handleWidth, y, handleWidth, h);
		pos2 = QPoint(pos.x(), pos.y() + wpos.y());
            }

      switch (_tool) {
            case PencilTool:
			if (r1.contains(pos2))
                  	state = S_START_DRAG1;
             	else if (r2.contains(pos2))
                  	state = S_START_DRAG2;
            	else
                  	state = S_START_DRAG5;
                  ppos   = pos2pix(pix2pos(startDrag.x()).downSnaped(raster()));
                  psize  = pos2pix(pix2pos(ppos+1).upSnaped(raster())) - ppos;
                  startDragTime = QTime::currentTime();
                  setCursor();
                  break;
            case RubberTool:
            	if (part)
                  	audio->msgRemovePart(part);
                  break;
            case GlueTool:
                  if (part)
                        song->cmdGluePart(part);
                  break;
            case CutTool:
                  if (part)
                        splitPart(part, pos);
                  break;
            case MuteTool:
                  if (part) {
                        part->setMute(!part->mute());
                        widget()->update();
                        }
                  break;
            default:
                  if (hit == HIT_PART) {
                        QRect r3(xpos, y, len, h);

                        if (r1.contains(pos2)) {
                              state = S_START_DRAG1;
                              }
                        else if (r2.contains(pos2)) {
                              state = S_START_DRAG2;
                              }
                        else if (r3.contains(pos2)) {
                              state = S_START_DRAG3;
                              bool add = keyState & Qt::ShiftModifier;
                              song->selectPart(part, add);
                              emit kbdMovementUpdate(track, part);
                              }
                        }
                  if (state == S_NORMAL) {
                        song->selectPart(0, false);   // deselect all parts
                        emit kbdMovementUpdate(0, 0); // Tell arranger nothing is selected (Keyboard movement)
                        }
                  startDragTime = QTime::currentTime();
                  setCursor();
                  break;
            }
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void PartCanvas::mouseMove(QPoint pos)
      {
      if (state == S_SUBTRACK) {
            TLSWidget* w = (TLSWidget*)(at->tw);
            int y = pos.y() - w->y() + 2 - rulerHeight;
            w->mouseMove(QPoint(pos.x(), y));
            return;
            }
      pos -= rCanvasA.topLeft();
      bool update = false;
      int x     = pos.x();
      int delta = startDrag.x() - x;
      int t     = startDragTime.msecsTo(QTime::currentTime());
      bool dragActive = (startDrag - pos).manhattanLength() >
                     QApplication::startDragDistance()
                     || t > QApplication::startDragTime();
      switch (state) {
            case S_START_DRAG1:
                  if (dragActive)
                        state = S_DRAG1;
                  break;
            case S_START_DRAG2:
                  if (dragActive)
                        state = S_DRAG2;
                  break;
            case S_START_DRAG3:
                  if (dragActive)
                        state = S_DRAG3;
                  break;
            case S_START_DRAG5:
                  if (dragActive)
                        state = S_DRAG5;
                  break;
            case S_NORMAL:
                  {
                  searchPart(pos);
                  bool found = false;
                  if (part) {
                        int h    = track->arrangerTrack.tw->height();
                        int xpos = mapx(part->tick());
                        int len  = rmapx(part->lenTick());
                        int y    = track->arrangerTrack.tw->y();
                        QRect r1(xpos,                     y, handleWidth, h);
                        QRect r2(xpos + len - handleWidth, y, handleWidth, h);
                        if (r1.contains(pos) || r2.contains(pos))
                              found = true;
                        }
                  if (found) {
                        widget()->setCursor(Qt::SizeHorCursor);
                        }
                  else
                        setCursor();
                  }
                  break;
            default:
                  break;
            }
      if (!track)
            return;
    	int y = track->arrangerTrack.tw->y() - splitWidth/2;
	int ph = track->arrangerTrack.tw->height() - 1 - partBorderWidth;
      if (state == S_DRAG1) {
            //
            // drag left edge of part
            //
            Pos p(pix2pos(ppos - delta));
            p.snap(raster());
            int x1  = pos2pix(p);
            int x2  = pos2pix(part->end());
            int size = x2 - x1;
            drag.setRect(x1, y, size, ph);
            update = true;
            }
      else if (state == S_DRAG2) {
            //
            // drag right edge of part
            //
            int size = psize - delta;
            if (size < 10)
                  size = 10;
            int x2 = mapx(AL::sigmap.raster(part->tick() + rmapxDev(size), raster()));
            drag.setRect(ppos, y, x2 - ppos, ph);
            update = true;
            }
      else if (state == S_DRAG5) {
            //
            // draw part with pencil tool
            //
            int size = psize - delta;
            if (size < 10)
                  size = 10;
            int x2 = mapx(AL::sigmap.raster(mapxDev(ppos + size), raster()));
            drag.setRect(ppos, y, x2 - ppos, ph);
            update = true;
            }
      else if (state == S_DRAG3) {
            //
            // drag whole part
            //
            QDrag* d = 0;
            if (track->type() == Track::MIDI)
                  d = new MidiPartDrag(part, this);
            else if (track->type() == Track::WAVE)
                  d = new AudioPartDrag(part, this);
            if (d) {
                  _dragOffset = startDrag.x() - rCanvasA.x() - ppos;
                  /* Qt::DropAction da =*/ d->start(Qt::CopyAction | Qt::LinkAction | Qt::MoveAction);
                  update = true;
                  }
            state = S_NORMAL;
            }
      if (update)
            widget()->update();
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void PartCanvas::mouseRelease(QMouseEvent* me)
      {
      if (state == S_SUBTRACK) {
            ((TLSWidget*)(at->tw))->mouseRelease();
            state = S_NORMAL;
            return;
            }

      QPoint pos(me->pos());
      int x = pos.x();
      int delta = startDrag.x() - x;

      if (state == S_DRAG1) {
            int val  = mapxDev(ppos-delta);
            int pos  = AL::sigmap.raster(val, raster());
            int size = part->tick() + part->lenTick() - pos;
            emit partChanged(part, pos, size);
            }
      else if (state == S_DRAG2) {
            int size = psize - delta;
            int x1   = part->tick();
            int x2   = AL::sigmap.raster(part->tick() + rmapxDev(size), raster());

            int step = AL::sigmap.rasterStep(x1, raster());
            if (x2 - x1 < step)
                  x2  = AL::sigmap.raster(x1 + step, raster());
            emit partChanged(part, x1, x2-x1);
            }
      else if (state == S_DRAG5) {
            if (track && (track->type() == Track::MIDI || track->type() == Track::WAVE)) {
                  Part* part = track->newPart();
                  Pos p1 = pix2pos(drag.x()).snaped(raster());
                  Pos p2 = pix2pos(drag.x() + drag.width()).snaped(raster());
                  part->setPos(p1);
                  part->setLenTick(p2.tick() - p1.tick());
                  song->addPart(part);
                  }
            else
                  widget()->update();
            }
      state = S_NORMAL;
      setCursor();
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void PartCanvas::mouseDoubleClick(QMouseEvent* me)
      {
      QPoint pos(me->pos().x(), me->pos().y() - rulerHeight);
      if (_tool != PointerTool) {
//TD            mousePress(pos);
            return;
            }
      searchPart(pos);
      bool shift  = keyState & Qt::ShiftModifier;
      if (part) {
            if (button == Qt::LeftButton && shift) {
#if 0 //TODO1
                  editPart = (NPart*)curItem;
                  QRect r = map(curItem->bbox());
                  if (lineEditor == 0) {
                        lineEditor = new QLineEdit(this);
                        lineEditor->setFrame(true);
                        }
                  editMode = true;
                  lineEditor->setGeometry(r);
                  lineEditor->setText(editPart->name());
                  lineEditor->setFocus();
                  lineEditor->show();
#endif
                  }
            else if (button == Qt::LeftButton) {
                  emit doubleClickPart(part);
                  }
            }
      //
      // double click creates new part between left and
      // right mark

      else if (track && track->isMidiTrack())
           emit createLRPart(track);
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void PartCanvas::setCursor()
      {
      switch(state) {
            case S_START_DRAG1:
            case S_START_DRAG2:
            case S_DRAG1:
            case S_DRAG2:
                  widget()->setCursor(Qt::SizeHorCursor);
                  return;
            default:
                  break;
            }
      TimeCanvas::setCursor();
      }

//---------------------------------------------------------
//   declonePart
//---------------------------------------------------------

void PartCanvas::declonePart(Part* spart)
      {
      Track* track = spart->track();
      Part* dpart  = track->newPart(spart, false);

      EventList* se = spart->events();
      EventList* de = dpart->events();
      for (iEvent i = se->begin(); i != se->end(); ++i) {
            Event oldEvent = i->second;
            Event ev = oldEvent.clone();
            de->add(ev);
            }
      audio->msgChangePart(spart, dpart, true);
      track->partListChanged();
      }

//---------------------------------------------------------
//   splitPart
//---------------------------------------------------------

void PartCanvas::splitPart(Part* part, const QPoint& p)
      {
      song->cmdSplitPart(part, pix2pos(p.x()).snaped(raster()));
      }

//---------------------------------------------------------
//   renamePart
//---------------------------------------------------------

void PartCanvas::renamePart(Part*)
      {
      printf("rename part: not impl.\n");
      }

//---------------------------------------------------------
//   cutPart
//---------------------------------------------------------

void PartCanvas::cutPart(Part*)
      {
      printf("cut part: not impl.\n");
      }

//---------------------------------------------------------
//   copyPart
//---------------------------------------------------------

void PartCanvas::copyPart(Part*)
      {
      printf("copy part: not impl.\n");
      }

//---------------------------------------------------------
//   dragEnter
//---------------------------------------------------------

void PartCanvas::dragEnter(QDragEnterEvent* event)
      {
      const QMimeData* md = event->mimeData();
      if (MidiPartDrag::canDecode(md)
         || AudioPartDrag::canDecode(md)
         || WavUriDrag::canDecode(md)) {
            event->acceptProposedAction();
            }
      else {
            QStringList formats = md->formats();
            foreach(QString s, formats)
                  printf("drag format <%s>\n", s.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PartCanvas::dragMove(QDragMoveEvent* event)
      {
      Part* srcPart = 0;
      QString filename;

      const QMimeData* md = event->mimeData();
      if (MidiPartDrag::canDecode(md)) {
            MidiPartDrag::decode(md, srcPart);
            }
      else if (AudioPartDrag::canDecode(md)) {
            AudioPartDrag::decode(md, srcPart);
            }
      else if (WavUriDrag::canDecode(md)) {
            WavUriDrag::decode(md, &filename);
            }
      else {
            state = S_NORMAL;
            event->ignore();
            return;
            }
      Track* srcTrack = srcPart ? srcPart->track() : 0;

      QPoint p(event->pos() - rCanvasA.topLeft());
      searchPart(p);
      if (!track) {
            if (state != S_NORMAL) {
                  state = S_NORMAL;
                  widget()->update();
                  }
            event->ignore();
            return;
            }
      if (srcTrack == 0) {    // drag uri
            if (state != S_NORMAL) {
                  state = S_NORMAL;
                  widget()->update();
                  }
            if (track->type() == Track::WAVE)
                  event->acceptProposedAction();
            else
                  event->ignore();
            return;
            }
      if (track->type() != srcTrack->type()) {
            if (state != S_NORMAL) {
                  state = S_NORMAL;
                  widget()->update();
                  }
            event->ignore();
            return;
            }
      event->acceptProposedAction();
      state = S_DRAG4;
      ArrangerTrack* at = &(track->arrangerTrack);
      int y = at->tw->y() - splitWidth/2;
//      int h = at->tw->height();

      PartCanvas* cw = (PartCanvas*)event->source();
      QRect updateRect(drag);

	Pos pos(pix2pos(p.x() - cw->dragOffset()).snaped(raster()));
	int x = pos2pix(pos);
      drag.setRect(
         x,
         y,
         rmapx(srcPart->lenTick()),
         at->tw->height() - 1 - partBorderWidth
         );
	updateRect |= drag;
      updateRect.adjust(-1, -1 + rCanvasA.y(), 1, 1 + rCanvasA.y());
      widget()->update(updateRect);
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PartCanvas::drop(QDropEvent* event)
      {
      state = S_NORMAL;
      Part* srcPart = 0;
      QString filename;

      const QMimeData* md = event->mimeData();
      if (MidiPartDrag::canDecode(md)) {
            MidiPartDrag::decode(md, srcPart);
            }
      else if (AudioPartDrag::canDecode(md)) {
            AudioPartDrag::decode(md, srcPart);
            }
      else if (WavUriDrag::canDecode(md)) {
            WavUriDrag::decode(md, &filename);
            }
      else
            return;

      QPoint pos(event->pos() - rCanvasA.topLeft());
      searchPart(pos);
      Track* srcTrack = srcPart ? srcPart->track() : 0;
      if (track == 0 || (srcTrack && (track->type() != srcTrack->type())))
            return;

      if (srcPart == 0) {
            int tick = AL::sigmap.raster(mapxDev(pos.x()), raster());
            Pos pos(tick);
            muse->importWaveToTrack(filename, track, pos);
            }
      else {
            PartCanvas* cw = (PartCanvas*)event->source();
            unsigned tick = AL::sigmap.raster(mapxDev(pos.x() - cw->dragOffset()), raster());
            if (srcPart->tick() != tick || srcTrack != track) {
                  Qt::KeyboardModifiers keyState = event->keyboardModifiers();

                  if (keyState & Qt::ShiftModifier) {
                        song->copyPart(srcPart, tick, track);
                        event->setDropAction(Qt::CopyAction);
                        }
                  else if (keyState & Qt::ControlModifier) {
                        song->linkPart(srcPart, tick, track);
                        event->setDropAction(Qt::LinkAction);
                        }
                  else {
                        song->movePart(srcPart, tick, track);
                        event->setDropAction(Qt::MoveAction);
                        }
                  }
            }
      event->acceptProposedAction();
      widget()->update();
      }

//---------------------------------------------------------
//   dragLeave
//---------------------------------------------------------

void PartCanvas::dragLeave(QDragLeaveEvent*)
      {
      if (state == S_DRAG4) {
            state = S_NORMAL;
            widget()->update();
            }
      }

