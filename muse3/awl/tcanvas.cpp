//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include "tcanvas.h"
#include "al/al.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "../muse/gconfig.h"
#include "../muse/icons.h"

#include "metronom.xpm"
#include "clock.xpm"

#ifdef __APPLE__
  inline double exp10(double a) { return pow(10.0, a); }
#endif

static QIcon* clockIcon;
static QIcon* metronomIcon;

FollowMode TimeCanvas::followMode = FOLLOW_JUMP;
QPixmap* TimeCanvas::octave;
QPixmap* TimeCanvas::mk1;
QPixmap* TimeCanvas::mk2;
QPixmap* TimeCanvas::mk3;
QPixmap* TimeCanvas::mk4;

enum DragType {
      DRAG_RULER,
      DRAG_CANVASA, DRAG_CANVAS_B,
      DRAG_PANELA, DRAG_PANELB,
      DRAG_OTHER
      };

//---------------------------------------------------------
//   TimeCanvas
//---------------------------------------------------------

TimeCanvas::TimeCanvas(TimeCanvasType t)
   : QFrame()
      {
      setAttribute(Qt::WA_NoSystemBackground, true);

      _yFit      = false;
      _tool      = PointerTool;
      type       = t;
      _timeType  = AL::TICKS;
      marker     = 0;
      showCursor = false;
      ctrlHeight = 0;
      curPitch   = -1;
      mouseInB   = false;
      dragType   = DRAG_OTHER;
      followPos  = true;

      // set default color
      canvasBackgroundColor = QColor(0x71, 0x8d, 0xbe);

      if (clockIcon == 0) {
            clockIcon = new QIcon(QPixmap(clock_xpm));
            metronomIcon = new QIcon(QPixmap(metronom_xpm));
            }
      button = Qt::NoButton;
      grid   = new QGridLayout;
      grid->setMargin(0);
      grid->setSpacing(1);
      setLayout(grid);

      _widget = new QWidget;
      _widget->setAttribute(Qt::WA_NoSystemBackground);
      _widget->setAttribute(Qt::WA_StaticContents);
      _widget->installEventFilter(this);
      _widget->setMouseTracking(true);
      _widget->setAcceptDrops(true);

      // allow to set slider position before slider range
      // is known:

      pos1.setTick(0);
      pos2.setTick(INT_MAX);

      hmag = new QSlider(Qt::Horizontal);
      hmag->setRange(0, 100);
      _xmagMin = 0.001;
      _xmagMax = 0.3;
      _xmag    = 0.04;
      hmag->setValue(xmag2s(_xmag));

      vmag = 0;
      if (type != TIME_CANVAS_DRUMEDIT) {
            vmag = new QSlider(Qt::Vertical);
            vmag->setRange(0, 100);
            vmag->setPageStep(1);
            }
      _ymag = 1.0;

      hbar = new QScrollBar(Qt::Horizontal);
      hbar->setRange(0, INT_MAX);
      vbar = new QScrollBar(Qt::Vertical);
      timeTypeButton = new QToolButton;
      timeTypeButton->setFixedSize(20, rulerHeight);
      setTimeType1(AL::TICKS);
      yRange = 0;

      switch(type) {
            case TIME_CANVAS_PIANOROLL:
                  _ymagMin = 0.5;
                  _ymagMax = 3.0;
                  vmag->setValue(lrint((_ymag-_ymagMin)*100.0/(_ymagMax-_ymagMin)));
                  initPianoroll();
                  break;
            case TIME_CANVAS_DRUMEDIT:
                  _ymagMin = 1.0;
                  _ymagMax = 1.0;
                  yRange   = drumHeight * 128;
                  break;
            case TIME_CANVAS_WAVEEDIT:
                  _xmagMin = 0.001;
                  _xmagMax = 100.0;
                  _xmag    = 0.04;
                  _ymagMin = 1.0;
                  _ymagMax = 10.0;
                  _ymag    = 1.0;
                  break;
            default:
                  _ymagMin = 1.0;
                  _ymagMax = 1.0;
                  break;
            }
      updateGeometry();
      if (type == TIME_CANVAS_PIANOROLL || type == TIME_CANVAS_DRUMEDIT
         || type == TIME_CANVAS_WAVEEDIT) {
            addCtrlButton = new QPushButton(tr("Ctrl"), _widget);
            addCtrlButton->setGeometry(1, 1, rPanelA.width()-4, rulerHeight-4);
            addCtrlButton->setToolTip(tr("Add Controller View"));
            connect(addCtrlButton, SIGNAL(clicked()), SLOT(addCtrlClicked()));
            }

      grid->addWidget(_widget,  0, 0, 3, 2);
      grid->addWidget(hbar,     3, 0, Qt::AlignVCenter);
      grid->addWidget(hmag,     3, 1, Qt::AlignVCenter);
      grid->addWidget(timeTypeButton, 0, 2);
      grid->addWidget(vbar,     1, 2, Qt::AlignHCenter);
      if (vmag)
            grid->addWidget(vmag,     2, 2, Qt::AlignHCenter);

      grid->setColumnStretch(0, 100);
      grid->setRowStretch(1, 100);

      _raster = 0;
      updateScrollBars();
      connect(hbar, SIGNAL(valueChanged(int)), SLOT(moveX(int)));
      connect(vbar, SIGNAL(valueChanged(int)), SLOT(moveY(int)));
      connect(hmag, SIGNAL(valueChanged(int)), SLOT(scaleX(int)));
      if (vmag)
            connect(vmag, SIGNAL(valueChanged(int)), SLOT(scaleY(int)));
      connect(timeTypeButton, SIGNAL(clicked()), SLOT(toggleTimeType()));
      }

//---------------------------------------------------------
//   resizeController
//---------------------------------------------------------

void TimeCanvas::resizeController(int h)
      {
      if (h == ctrlHeight)
            return;
      int updateH = h > ctrlHeight ? h : ctrlHeight;
      ctrlHeight = h;
      updateGeometry();
      updateScrollBars();
      widget()->update(0, widget()->height() - updateH, widget()->width(), updateH);
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool TimeCanvas::eventFilter(QObject* obj, QEvent* event)
      {
      if (obj != _widget)
            return QFrame::eventFilter(obj, event);

      switch(event->type()) {
            case QEvent::Paint:
                  {
                  QPainter p(_widget);
                  canvasPaintEvent(((QPaintEvent*)event)->rect(), p);
                  }
                  return true;

            case QEvent::Resize:
                  updateGeometry();
                  updateScrollBars();
                  layout();
                  return false;

            case QEvent::MouseButtonDblClick:
                  {
                  QMouseEvent* me = (QMouseEvent*)event;
                  QPoint p(me->pos());
                  button   = me->button();
                  keyState = me->modifiers();
                  mouseDoubleClick(me);
                  }
                  return true;

            case QEvent::MouseButtonPress:
                  {
                  QMouseEvent* me = (QMouseEvent*)event;
                  keyState = me->modifiers();
                  button   = me->button();
                  QPoint p(me->pos());
                  int x = p.x() - rRuler.x();
                  bool shift = keyState & Qt::ShiftModifier;

                  if (rRuler.contains(p)) {
                        dragType = DRAG_RULER;
                        if (shift) {
                              AL::Pos pos(pix2pos(x));
                              if (button == Qt::LeftButton)
                                    emit addMarker(pos);
                              else if (button == Qt::RightButton)
                                    emit removeMarker(pos);
                              return true;
                              }
                        }
                  else {
                        dragType = DRAG_OTHER;
                        mousePress(me);
                        }
                  }
                  // go on with MouseMove

            case QEvent::MouseMove:
                  {
                  QMouseEvent* me = (QMouseEvent*)event;
                  keyState        = me->modifiers();
                  button          = me->buttons();
                  QPoint p(me->pos());
                  AL::Pos pos(pix2pos(p.x()-rCanvasA.x()));

                  if (dragType == DRAG_OTHER) {
                        if (button == 0 && (rPanelB.contains(p) || rCanvasB.contains(p))) {
                              if (!mouseInB) {
                                    mouseInB = true;
                                    enterB();
                                    }
                              }
                        else {
                              if (button == 0 && mouseInB) {
                                    mouseInB = false;
                                    leaveB();
                                    }
                              }

                        if (showCursor && p.x() < rCanvasA.x()) {
                              showCursor = false;
                              widget()->update(rRuler);
                              emit cursorPos(cursor, showCursor);
                              }

                        if (p.x() >= rCanvasA.x() && (cursor != pos)) {
					int x1 = pos2pix(cursor) + rCanvasA.x();
                              int x2 = pos2pix(pos)    + rCanvasA.x();
                              QRect r1(x1-1, 0, 2, rRuler.height());
                              QRect r2(x2-1, 0, 2, rRuler.height());
                              widget()->update(rRuler & (r1 | r2));
                              cursor     = pos;
                              showCursor = true;
                              emit cursorPos(cursor, showCursor);
                              }

                        if (rRuler.contains(p)) {
                              int b = me->buttons();
                              if (b == 0)
                                    return true;
                              int i = 0;
                              if (b & Qt::MidButton)
                                    i = 1;
                              //else if (b & Qt::RightButton)
                              //      i = 2;
                              else if (b & Qt::RightButton) {
                                    if ((MusEGlobal::config.rangeMarkerWithoutMMB) && (event->modifiers() & Qt::ControlModifier))
                                        i = 1;
                                    else
                                        i = 2;
                                    }
                              if (keyState & Qt::ShiftModifier)
                                    emit addMarker(i);
                              emit posChanged(i, pos);
                              }
                        else {
                              mouseMove(p);
                              }
                        }
                  else if (dragType == DRAG_RULER) {
                        int b = me->buttons();
                        if (b == 0)
                              return true;
                        int i = 0;
                        if (b & Qt::MidButton)
                              i = 1;
                        else if (b & Qt::RightButton)
                              i = 2;
                        if (keyState & Qt::ShiftModifier)
                              emit addMarker(i);
                        emit posChanged(i, pos);
                        }
                  }
                  return true;

            case QEvent::MouseButtonRelease:
                  {
                  QMouseEvent* me = (QMouseEvent*)event;
                  button   = Qt::NoButton;
                  keyState = me->modifiers();
                  mouseRelease(me);
      		dragType = DRAG_OTHER;
                  }
                  return true;

            case QEvent::DragEnter:
                  dragEnter((QDragEnterEvent*)event);
                  return true;

            case QEvent::Drop:
                  drop((QDropEvent*)event);
                  return true;

            case QEvent::DragMove:
                  dragMove((QDragMoveEvent*)event);
                  return true;

            case QEvent::DragLeave:
                  dragLeave((QDragLeaveEvent*)event);
                  return true;

            case QEvent::Leave:
            	{
                  emit cursorPos(cursor, false);
                  showCursor = false;
                  emit pitchChanged(-1);
                  curPitch = -1;
            	QRect r(rRuler);
            	if (!rPanelA.isEmpty())
                  	r |= rPanelA;
                  widget()->update(r);

                  if (mouseInB) {
                        mouseInB = false;
                        // button = ((QMouseEvent*)event)->buttons();
                        leaveB();
                        }
                  }
                  return false;

            case QEvent::Wheel:
            	{
                  QWheelEvent* e = (QWheelEvent*)event;
                  if (e->orientation() != Qt::Vertical)
            		return true;
      		if ((e->modifiers() & Qt::ControlModifier) || (e->modifiers() & Qt::ShiftModifier)) {
                        //
                        // xmag
                        //
                        int oldx = e->x() - rCanvasA.x();
                        AL::Pos pos(pix2pos(oldx));
                        int step = e->delta() / 120;
                        if (step > 0) {
                              for (int i = 0; i< step; ++i)
                                    _xmag *= 1.1;
                              }
                        else {
                              for (int i = 0; i < -step; ++i)
                                    _xmag *= 0.9;
                              }
                        if (_xmag < _xmagMin)
                              _xmag = _xmagMin;
                        else if (_xmag > _xmagMax)
                              _xmag = _xmagMax;
                        hmag->setValue(xmag2s(_xmag));
                        int newx = pos2pix(pos);
                        updateScrollBars();
                        hbar->setValue(wpos.x() + (newx - oldx));
                        updateRulerMag();
                        magChanged();
                        _widget->update();
                        }
                  else {
                        //
                        //   scroll
                        //
                        int step = qMin(QApplication::wheelScrollLines() * vbar->singleStep(), vbar->pageStep());
            		int offset = e->delta() * step / 120;
            		if (vbar->invertedControls())
                  		offset = -offset;
            		if (qAbs(offset) < 1)
                  		return true;
      	      	vbar->setValue(vbar->value() + offset);
                        }
                  }
            	return true;
            default:
// printf("event %d missed\n", event->type());
                  break;
            }
      return false;
      }



void TimeCanvas::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Up   || e->key() == Qt::Key_Down ||
        e->key() == Qt::Key_Left || e->key() == Qt::Key_Right)
        keyboardNavigate(e);
}

//---------------------------------------------------------
//   moveX
//---------------------------------------------------------

void TimeCanvas::moveX(int x)
      {
      int dx = wpos.x() - x;
      wpos.setX(x);

      int wh = _widget->height();

      if (type == TIME_CANVAS_PIANOROLL || type == TIME_CANVAS_DRUMEDIT
         || TIME_CANVAS_WAVEEDIT) {
            _widget->scroll(dx, 0, QRect(rCanvasA.x(), 0, rCanvasA.width(), wh));

            //HACK:
            // update controller names
            int w = 100 + ((dx > 0) ? dx : 0);
            _widget->update(rCanvasB.x(), rCanvasB.y(), w, rCanvasB.height());

            //HACK:
            // repaint rounded line end (splitter handle for controller
            // canvas)
            int x = rCanvasB.x() + rCanvasB.width() - 1;
            w = 1;
            if (dx < 0) {
                  x += dx;
                  w -= dx;
                  }
            _widget->update(x, rCanvasB.y(), w, rCanvasB.height());
            }
      else
            _widget->scroll(dx, 0);
      emit contentsMoving(wpos.x(), wpos.y());
      }

//---------------------------------------------------------
//   moveY
//---------------------------------------------------------

void TimeCanvas::moveY(int y)
      {
      int dy = wpos.y() - y;
      if (dy == 0)
            return;
      wpos.setY(y);

      // dont move ruler:

      int ww = _widget->width();
      int wh = _widget->height();

      QRect r(0, rulerHeight, ww, wh - rulerHeight - ctrlHeight);

      _widget->scroll(0, dy, r);
      emit contentsMoving(wpos.x(), wpos.y());
      }

//---------------------------------------------------------
//   setYPos
//---------------------------------------------------------

void TimeCanvas::setYPos(int y)
      {
      setWPos(QPoint(wpos.x(), y));
      }

//---------------------------------------------------------
//   setWPos
//---------------------------------------------------------

void TimeCanvas::setWPos(const QPoint& p)
      {
      if (wpos != p) {
            wpos = p;
            hbar->setValue(wpos.x());
            vbar->setValue(wpos.y());
            _widget->update();
//            QCoreApplication::flush();
            }
      }

//---------------------------------------------------------
//   paintClockRuler
//---------------------------------------------------------

void TimeCanvas::paintClockRuler(QPainter& p, const QRect& r)
      {
      int x1 = r.x();
      int x2 = x1 + r.width();

      int y1 = r.y();
      int rh = r.height();
      if (y1 < rulerHeight) {
            rh -= rulerHeight - y1;
            y1 = rulerHeight;
            }
      int y2 = y1 + rh;

      //---------------------------------------------------
      //    draw Marker
      //---------------------------------------------------

      int y = rulerHeight - 16;
      p.setPen(Qt::black);
      p.setFont(_font3);
      QRect tr(r);
      tr.setHeight(12);

      if (marker) {
            for (AL::iMarker m = marker->begin(); m != marker->end(); ++m) {
                  int xp = mapx(int(m->second.frame()));
                  if (xp > x2)
                        break;
                  AL::iMarker mm = m;
                  ++mm;
                  int xe = x2;
                  if (mm != marker->end()) {
                        xe = mapx(mm->first);
                        }
                  QRect tr(xp, 0, x2 - xp, 11);
                  if (m->second.current()) {
                        p.fillRect(tr, Qt::white);
                        }
                  if (r.intersects(tr)) {
                        int x2;
                        AL::iMarker mm = m;
                        ++mm;
                        if (mm != marker->end())
                              x2 = mapx(mm->first);
                        else
                              x2 = xp+200;
                        QRect r  = QRect(xp+10, 0, x2-xp, 12);
                        p.drawPixmap(xp, 0, *flagIconS);
                        p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, m->second.name());
                        }
                  }
            }

      p.setPen(Qt::black);
      if (showCursor) {
            int xp = pos2pix(cursor);
            if (xp >= x1 && xp < x2)
                  p.drawLine(xp, 0, xp, rulerHeight);
            }

      AL::Pos p1(pix2pos(x1));
      AL::Pos p2(pix2pos(x2));

      int sec1 = p1.frame() / AL::sampleRate;
      int sec2 = (p2.frame() + AL::sampleRate - 1) / AL::sampleRate;

      int sw = lrint(AL::sampleRate * _xmag);

      if (sw > 20) {
            for (int sec = sec1; sec < sec2; ++sec) {
                  int min = sec / 60;
                  int sr  = sec % 60;

                  int yy;
                  QString s;
                  if (sr == 0) {
                        p.setFont(_font2);
                        s.sprintf("%d:00", min);
                        yy = y;
                        }
                  else {
                        p.setFont(_font1);
                        s.sprintf("%02d", sr);
                        yy = y + 7;
                        }
                  int xp = pos2pix(AL::Pos(sec * AL::sampleRate, AL::FRAMES));
// printf("  sec %d min %d sr %d  xp %d\n", sec, min, sr, xp);
                  p.setPen(Qt::black);
                  p.drawLine(xp, yy, xp, rulerHeight);
                  p.drawText(xp + 2, rulerHeight - 4, s);
                  p.setPen(sr == 0 ? Qt::lightGray : Qt::gray);
                  p.drawLine(xp, y1, xp, y2);
                  }
            }
      else {
            int min1 = sec1/60;
            int min2 = (sec2+59)/60;
            for (int min = min1; min < min2; ++min) {
                  QString s;
                  p.setFont(_font2);
                  s.sprintf("%d", min);
                  int xp = pos2pix(AL::Pos(min * AL::sampleRate * 60, AL::FRAMES));
                  p.setPen(Qt::black);
                  p.drawLine(xp, y, xp, rulerHeight);
                  p.drawText(xp + 2, rulerHeight - 4, s);
                  p.setPen(Qt::lightGray);
                  p.drawLine(xp, y1, xp, y2);
                  }
            }
      }

//---------------------------------------------------------
//   updateRulerMag
//---------------------------------------------------------

void TimeCanvas::updateRulerMag()
      {
      int bar1, beat, tick;
      pos1.mbt(&bar1, &beat, &tick);
      AL::Pos stick(bar1, 0, 0);
      AL::Pos ntick = AL::Pos(bar1 + 1, 0, 0);
      int tpix  = pos2pix(ntick) - pos2pix(stick);
      metronomeRulerMag = 0;
      if (tpix < 64)
      	metronomeRulerMag = 1;
      if (tpix < 32)
      	metronomeRulerMag = 2;
	if (tpix <= 16)
      	metronomeRulerMag = 3;
	if (tpix < 8)
      	metronomeRulerMag = 4;
      if (tpix <= 4)
      	metronomeRulerMag = 5;
      if (tpix <= 2)
      	metronomeRulerMag = 6;
      }

//---------------------------------------------------------
//   paintMetronomRuler
//---------------------------------------------------------

void TimeCanvas::paintMetronomRuler(QPainter& p, const QRect& r)
      {
      static const int mag[7] = {
            1, 1, 2, 5, 10, 20, 50
            };

      int x = r.x();
      int w = r.width();
      int y = rulerHeight - 16;

      p.setFont(_font3);

      int h  = 14;
      int y1 = r.y();
      int rh = r.height();
      if (y1 < rulerHeight) {
            rh -= rulerHeight - y1;
            y1 = rulerHeight;
            }
      int y2 = y1 + rh;

      if (x < (MAP_OFFSET - wpos.x()))
            x = MAP_OFFSET - wpos.x();
      AL::Pos pos1 = pix2pos(x);
      AL::Pos pos2 = pix2pos(x+w);

      if (marker) {
            AL::iMarker start = marker->lower_bound(pos1.tick());
            if (start != marker->begin())
                  --start;
            AL::iMarker end = marker->lower_bound(pos2.tick());
            for (AL::iMarker m = start; m != end; ++m) {
                  AL::Pos pm1(m->second);
                  AL::iMarker m2 = m;
                  ++m2;
                  AL::Pos pm2(pos2);
                  if (m2 != marker->end())
                        pm2 = m2->second;

                  int x1 = pos2pix(pm1);
                  int x2 = pos2pix(pm2);

                  if (pos[0] >= pm1 && (m2 == marker->end() || pos[0] < pm2))
                        p.fillRect(x1, 0, x2 - x1, 11, Qt::white);

                  QRect r  = QRect(x1 + 10, 0, x2 - x1, 12);
                  p.drawPixmap(x1, 0, *flagIconS);
                  p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, m->second.name());
                  }
            }

      //---------------------------------------------------
      //    draw raster
      //---------------------------------------------------

      int bar1, bar2, beat, tick;
      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      int n = mag[metronomeRulerMag];

	bar1 = (bar1 / n) * n;		// round down
      if (bar1 && n >= 2)
            bar1 -= 1;
	bar2 = ((bar2 + n - 1) / n) * n; // round up

      for (int bar = bar1; bar <= bar2;) {
      	AL::Pos stick(bar, 0, 0);
            if (metronomeRulerMag) {
			p.setFont(_font2);
                  int x = pos2pix(stick);
                  QString s;
                  s.setNum(bar + 1);

                  p.setPen(Qt::black);
                  p.drawLine(x, y, x, y + h);
                  QRect r = QRect(x+2, y, 1000, h);
                  p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, s);
                  p.setPen(Qt::lightGray);
                  if (x > 0)
                        p.drawLine(x, y1, x, y2);
                  }
            else {
                  AL::TimeSignature sig = stick.timesig();
                  int z = sig.z;
                  for (int beat = 0; beat < z; beat++) {
                        AL::Pos xx(bar, beat, 0);
                        int xp = pos2pix(xx);
                        if (xp < 0)
                              continue;
                        QString s;
                        QRect r(xp+2, y + 1, 1000, h);
                        int y3;
                        int num;
                        if (beat == 0) {
                              num = bar + 1;
                              y3  = y + 2;
                              p.setFont(_font2);
                              }
                        else {
                              num = beat + 1;
                              y3  = y + 8;
                              p.setFont(_font1);
                              r.moveTop(r.top() + 1);
                              }
                        s.setNum(num);
                        p.setPen(Qt::black);
                        p.drawLine(xp, y3, xp, y+h);
                        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, s);
                        p.setPen(beat == 0 ? Qt::lightGray : Qt::gray);
                        if (xp > 0)
                              p.drawLine(xp, y1, xp, y2);
                        }
                  }
            if (bar == 0 && n >= 2)
                  bar += (n-1);
            else
                  bar += n;
            }
      //
      //  draw mouse cursor marker
      //
      p.setPen(Qt::black);
      if (showCursor) {
            int xp = pos2pix(cursor);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, 0, xp, rulerHeight-1);
            }

      }

//---------------------------------------------------------
//   tempoChanged
//---------------------------------------------------------

void TimeCanvas::tempoChanged()
      {
      widget()->update(rCanvasA.x(), 0, rCanvasA.width(), widget()->height());
      }

//---------------------------------------------------------
//   canvasPaintEvent
//---------------------------------------------------------

void TimeCanvas::canvasPaintEvent(const QRect& r, QPainter& p)
      {
      if (r.intersects(rButton)) {
            p.fillRect(rButton, QColor(0xe0, 0xe0, 0xe0));
            p.setPen(QPen(Qt::black, 2));
            int y = rButton.y() + rButton.height() - 1;
            p.drawLine(rButton.x(), y, rButton.width(), y);
            }
      p.setRenderHint(QPainter::TextAntialiasing, true);

      QRect par = r & rPanelA;
      if (!(par.isEmpty() || rPanelA.isEmpty())) {
            if (type == TIME_CANVAS_DRUMEDIT) {
                  paintDrumList(p, par);
                  }
            else if (type == TIME_CANVAS_PIANOROLL) {
                  paintPiano(p, par);
                  }
            else if (type == TIME_CANVAS_WAVEEDIT) {
                  p.fillRect(par, QColor(0xe0, 0xe0, 0xe0));
                  }
            }

      QRect pbr(r & rPanelB);
      QRect hor(r & (rRuler | rCanvasA | rCanvasB));
      QRect car(r & rCanvasA);
      QRect cbr(r & rCanvasB);

      bool drawPanelB  = !(pbr.isEmpty() || rPanelB.isEmpty());
      bool drawRuler   = !(hor.isEmpty() || (rRuler.isEmpty() && rCanvasA.isEmpty() && rCanvasB.isEmpty()));
      bool drawCanvasA = !(car.isEmpty() || rCanvasA.isEmpty());
      bool drawCanvasB = !(cbr.isEmpty() || rCanvasB.isEmpty());

      //
      //  draw canvas background
      //

      p.setClipRect(r);
      p.setBrushOrigin(QPoint(car.x() + wpos.x(), car.y() + wpos.y()));
      if (drawCanvasA) {
            if (canvasBackgroundPixmap.isNull()) {
            	if (type == TIME_CANVAS_DRUMEDIT || type == TIME_CANVAS_PIANOROLL
                     || type == TIME_CANVAS_WAVEEDIT) {
                        QRect rr(car);
                        // paint inactive area different
                        // (darker)
                        QColor c = canvasBackgroundColor.darker(150);
                        int x1 = pos2pix(partPos1) + rCanvasA.x();
                        if (rr.x() < x1) {
                              QRect r(rr.x(), rr.y(), x1-rr.x(), rr.height());
                              p.fillRect(r, c);
                              rr.adjust(x1-rr.x(), 0, 0, 0);
                              }
                        int x2  = pos2pix(partPos2) + rCanvasA.x();
                        int xx2 = rr.x() + rr.width();
                        if (xx2 > x2) {
                              if (x2 < rr.x())
                                    x2 = rr.x();
                              QRect r(x2, rr.y(), xx2-x2, rr.height());
                              p.fillRect(r, c);
                              rr.adjust(0, 0, -(xx2-x2), 0);
                              }
                        if (!rr.isEmpty()) {
	            		p.fillRect(rr, canvasBackgroundColor);
                              }
                        }
                  else
              		p.fillRect(car, canvasBackgroundColor);
                  }
            else {
                  p.drawTiledPixmap(car, canvasBackgroundPixmap,
            	   car.topLeft() + QPoint(wpos));
                  }
            }

      if (drawCanvasB)
            p.fillRect(cbr, canvasBackgroundColor);

      //---------------------------------------------------
      //    draw Ruler
      //---------------------------------------------------

      if (drawRuler) {
            QRect rrr(r & rRuler);
            if (!rrr.isEmpty())
                  p.fillRect(rrr, QColor(0xe0, 0xe0, 0xe0));
            int x1 = hor.x();
            int x2 = x1 + hor.width();
            int y1 = rulerHeight - 17;
            int y2 = rulerHeight - 1;

            p.setPen(QPen(Qt::black, 1));
            p.drawLine(x1, y1, x2, y1);
            p.setPen(QPen(Qt::black, 2));
            p.drawLine(x1, y2, x2, y2);

            QPoint off(rRuler.topLeft());
            p.translate(off);
            if (_timeType == AL::TICKS)
                  paintMetronomRuler(p, hor.translated(-off));
            else
                  paintClockRuler(p, hor.translated(-off));
            p.translate(-off);
            }

      if (drawCanvasA) {
            p.setClipRect(car);
            paintCanvas(p, car);
            }
      p.setRenderHint(QPainter::Antialiasing, false);
      if (drawPanelB) {
            p.setClipRect(pbr);
            QPoint off(rPanelB.topLeft());
            p.translate(off);
            paintControllerPanel(p, pbr.translated(-off));
            p.translate(-off);
            }
      if (drawCanvasB) {
            p.setClipRect(cbr);
            QPoint off(rCanvasB.topLeft());
            p.translate(off);
            paintControllerCanvas(p, cbr.translated(-off));
            p.translate(-off);
            }
      //---------------------------------------------------
      // draw marker
      //---------------------------------------------------

      int y1 = r.y();
      int y2 = y1 + r.height();
      if (drawRuler) {
            p.setClipRect(hor);
            int w  = r.width();
            int x  = r.x();
            int y  = rulerHeight - 16;
            QColor lcColors[3] = { Qt::red, Qt::blue, Qt::blue };

            for (int i = 0; i < 3; ++i) {
                  p.setPen(lcColors[i]);
                  int xp      = pos2pix(pos[i]) + rRuler.x();
                  QPixmap* pm = markIcon[i];
                  int pw = (pm->width() + 1) / 2;
                  int x1 = x - pw;
                  int x2 = x + w + pw;
                  if (xp >= x1 && xp < x2) {
                        p.drawPixmap(xp - pw, y-2, *pm);
                        p.drawLine(xp, y1, xp, y2);
                        }
                  }
            }
      if (marker) {
            int yy1 = y1;
            if (yy1 < rCanvasA.x())
                  yy1 = rCanvasA.x();
            p.setPen(Qt::green);
            AL::iMarker start = marker->lower_bound(pos1.tick());
            if (start != marker->begin())
                  --start;
            AL::iMarker end = marker->lower_bound(pos2.tick());
            if (end != marker->end())
                  ++end;
            for (AL::iMarker m = start; m != end; ++m) {
                  AL::Pos pm(m->second);
                  int x = pos2pix(pm) + rRuler.x();
                  p.drawLine(x, yy1, x, y2);
                  }
            }
      }

//---------------------------------------------------------
//   paintCanvas
//---------------------------------------------------------

void TimeCanvas::paintCanvas(QPainter& p, const QRect& cr)
      {
      QPoint off(rCanvasA.topLeft());

      if (type == TIME_CANVAS_PIANOROLL) {
            paintPianorollHorizontalGrid(p, cr);
            p.setRenderHint(QPainter::Antialiasing, true);
            }
      else if (type == TIME_CANVAS_DRUMEDIT) {
            paintDrumeditHorizontalGrid(p, cr);
            p.setRenderHint(QPainter::Antialiasing, true);
            }
      else
            off = QPoint(rCanvasA.x(), rCanvasA.y() - wpos.y());
      p.translate(off);
      paint(p, cr.translated(-off));
      p.resetMatrix();
      }

//---------------------------------------------------------
//   setLocatorPos
//---------------------------------------------------------

void TimeCanvas::setLocatorPos(int idx, const AL::Pos& val, bool follow)
      {
      if (pos[idx] == val)
            return;
      QFontMetrics fm(_font2);
      int fw  = fm.width("123") + 2;
      int w   = qMax(markIcon[idx]->width() + 2, fw);
      int h   = widget()->height();

      int x = pos2pix(val);
      if (idx == 0 && follow && followPos && followMode != FOLLOW_NO) {
            int scroll = 0;
            if (followMode == FOLLOW_JUMP) {
                  int x2 = rRuler.width() - 20;
                  if (x2 < 0)
                        x2 = rRuler.width();
                  if (x > x2) {
                        int x1 = 20;
                        if (x1 >= rRuler.width())
                              x1 = 0;
                        scroll = x - x1;
                        }
                  else if (x < 0) {
                        scroll = x - MAP_OFFSET;
                        }
                  }
            else if (followMode == FOLLOW_CONTINUOUS) {
                  int x1 = rRuler.width() / 2;
                  if (x != x1) {
                        scroll = x - (rRuler.width() / 2);
                        }
                  }
            if (scroll) {
                  moveX(wpos.x() + scroll);
                  hbar->setValue(wpos.x());
                  }
            }

      int offset = rRuler.x() - (w/2);
      int x1     = pos2pix(pos[idx]);
      int x2     = pos2pix(val);
      QRect oR(x1 + offset, 0, w, h);
      QRect nR(x2 + offset, 0, w, h);
      pos[idx] = val;
      widget()->update(oR | nR);
      }

//---------------------------------------------------------
//   setMag
//---------------------------------------------------------

void TimeCanvas::setMag(double x, double y)
      {
      if (_xmag == x && _ymag == y)
            return;
      _xmag = x;
      _ymag = y;
      if (vmag)
            vmag->setValue(lrint((_ymag-_ymagMin)*100.0/(_ymagMax-_ymagMin)));
      hmag->setValue(xmag2s(_xmag));
      updateScrollBars();
      updateRulerMag();
      magChanged();
      _widget->update();
      }

//---------------------------------------------------------
//   scaleX
//---------------------------------------------------------

void TimeCanvas::scaleX(int val)
      {
      _xmag = s2xmag(val);
      updateScrollBars();
      updateRulerMag();
      magChanged();
      _widget->update();
      }

//---------------------------------------------------------
//   scaleY
//---------------------------------------------------------

void TimeCanvas::scaleY(int val)
      {
      int y = lrint(wpos.y() / _ymag);
      _ymag = (_ymagMax - _ymagMin) / 100.0 * val + _ymagMin;
      y = lrint(y * _ymag);
      wpos.setY(y);
      updateScrollBars();
      magChanged();
      _widget->update();
      }

//---------------------------------------------------------
//   setRaster
//    r = 1   - no raster
//        0   - measure raster
//        > 1 - tick raster
//---------------------------------------------------------

void TimeCanvas::setRaster(int r)
      {
      if (_raster != r) {
            _raster = r;
            _widget->update();
            }
      }

//---------------------------------------------------------
//   setTimeRange
//---------------------------------------------------------

void TimeCanvas::setTimeRange(const AL::Pos& p1, const AL::Pos& p2)
      {
      if (pos1 == p1 && pos2 == p2)
            return;
      pos1 = p1;
      pos2 = p2;
      updateScrollBars();
      widget()->update();
      }

//---------------------------------------------------------
//   setEndPos
//---------------------------------------------------------

void TimeCanvas::setEndPos(const AL::Pos& p2)
      {
      if (pos2 == p2)
            return;
      pos2 = p2;
      updateScrollBars();
      widget()->update();
      }

//---------------------------------------------------------
//   updateScrollBars
//---------------------------------------------------------

void TimeCanvas::updateScrollBars()
      {
      hbar->blockSignals(true);
      vbar->blockSignals(true);

      int ymax = lrint(yRange * _ymag) - rCanvasA.height();
      if (ymax < 0)
            ymax = 0;
      vbar->setRange(0, ymax);
      vbar->setPageStep(rCanvasA.height());

      int xmin    = lrint(pos1.time(_timeType) * _xmag);
      unsigned x2 = pos2.time(_timeType);
      int xmax    = lrint(x2 * _xmag) - rCanvasA.width();
      if (xmax - xmin < 0)
            xmax = xmin;
      hbar->setRange(xmin, xmax);
      hbar->setPageStep(rCanvasA.width());
      wpos.setX(hbar->value());
      wpos.setY(vbar->value());

      hbar->blockSignals(false);
      vbar->blockSignals(false);
      }

//---------------------------------------------------------
//   setTimeType1
//---------------------------------------------------------

void TimeCanvas::setTimeType1(AL::TType t)
      {
      double conv = 1.0;
      if (t == AL::TICKS) {
            timeTypeButton->setIcon(*metronomIcon);
            if (_timeType == AL::FRAMES)
                  conv = AL::sampleRate / double(AL::division * 120 / 60);
            }
      else {
            timeTypeButton->setIcon(*clockIcon);
            if (_timeType == AL::TICKS)
                  conv = double(AL::division * 120 / 60) / double(AL::sampleRate);
            }
      _timeType = t;
      _xmag    *= conv;
      _xmagMax *= conv;
      _xmagMin *= conv;

      updateRulerMag();
      magChanged();
      }

//---------------------------------------------------------
//   setTimeType
//---------------------------------------------------------

void TimeCanvas::setTimeType(AL::TType t)
      {
      setTimeType1(t);
      updateScrollBars();
      timeTypeChanged();
      widget()->update();
      }

//---------------------------------------------------------
//   toggleTimeType
//---------------------------------------------------------

void TimeCanvas::toggleTimeType()
      {
      if (_timeType == AL::TICKS)
            setTimeType(AL::FRAMES);
      else
            setTimeType(AL::TICKS);
      }

//---------------------------------------------------------
//   setMarkerList
//---------------------------------------------------------

void TimeCanvas::setMarkerList(AL::MarkerList* ml)
      {
      if (marker == ml)
            return;
      marker = ml;
      widget()->update();
      }

//---------------------------------------------------------
//   pix2pos
//---------------------------------------------------------

AL::Pos TimeCanvas::pix2pos(int x) const
      {
      int val = lrint((x + wpos.x() - MAP_OFFSET)/_xmag);
      if (val < 0)
           val = 0;
      return AL::Pos(val, _timeType);
      }

//---------------------------------------------------------
//   pos2pix
//---------------------------------------------------------

int TimeCanvas::pos2pix(const AL::Pos& p) const
      {
      return lrint(p.time(_timeType) * _xmag) + MAP_OFFSET - wpos.x();
      }

//---------------------------------------------------------
//   mapx
//---------------------------------------------------------

int TimeCanvas::mapx(int x)  const
      {
      return lrint(x * _xmag) + MAP_OFFSET - wpos.x();
      }

//---------------------------------------------------------
//   mapxDev
//---------------------------------------------------------

int TimeCanvas::mapxDev(int x) const
      {
      int val = lrint((x + wpos.x() - MAP_OFFSET)/_xmag);
      if (val < 0)
           val = 0;
      return val;
      }

//---------------------------------------------------------
//   setCorderWidget
//---------------------------------------------------------

void TimeCanvas::setCornerWidget(QWidget* w)
      {
      grid->addWidget(w, 3, 2);
      }

//---------------------------------------------------------
//   initPianoroll
//---------------------------------------------------------

/*
      0   1   2  3  4  5  6  7  8  9  10
      c-2 c-1 C0 C1 C2 C3 C4 C5 C6 C7 C8 - G8

      Grid ve:

           +------------+ ------------------------------
       11  |            |
           |         b  |         7
           +------+     |
       10  |  a#  +-----+ ..............................
           +------+  a  |
        9  |            |         6
           +------+     |
        8  |  g#  +-----+ ..............................
           +------+  g  |
        7  |            |         5
           +------+     |
        6  |  f#  +-----+ ..............................
           +------+  f  |
        5  |            |         4
           |            |
           +------------+ ------------------------------
        4  |            |
           |         e  |         3
           +------+     |
        3  |  d#  +-----+ ..............................
           +------+  d  |
        2  |            |         2
           +------+     |
        1  |  c#  +-----+ ..............................
           +------+  c  |
           |            |         1
        0  |            |
           +------------+ ------------------------------
 */

void TimeCanvas::initPianoroll()
      {
      static const char *oct_xpm[] = {
      // w h colors
            "40 91 2 1",
            ". c #f0f0f0",
            "# c #000000",
            //           x
            "####################################### ",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#", // 10
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#", //------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",
            "####################################### ",     // 7
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",     // 6
            ".......................................#",
            ".......................................#",
            ".......................................#", //------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",     // 7
            "####################################### ",
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",    // 6
            ".......................................#",
            ".......................................#",
            ".......................................#", //------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",    // 7
            "####################################### ",
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",    // 10
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            "####################################### ", //----------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",    // 9
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#", //------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",
            "####################################### ",   // 7
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",     // 6
            ".......................................#",
            ".......................................#",
            ".......................................#", //--------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",     // 7
            "####################################### ",
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",     // 10
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            };

      static const char *mk1_xpm[] = {
            "40 13 2 1",
            ". c #ff0000",
            "# c none",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            "#######################................#",
            "########################...............#",
            "########################...............#",
            "####################################### ",
            };

      static const char *mk2_xpm[] = {
            "40 13 2 1",
            ". c #ff0000",
            "# c none",
            "########################...............#",
            "########################...............#",
            "#######################................#", //------------------------
            ".......................................#",
            ".......................................#",
            ".......................................#",     // 6
            ".......................................#",
            ".......................................#",
            ".......................................#", //--------------------------
            "#######################................#",
            "########################...............#",
            "########################...............#",     // 7
            "####################################### ",
            };

      static const char *mk3_xpm[] = {
            "40 13 2 1",
            ". c #ff0000",
            "# c none",
            "########################...............#",
            "########################...............#",
            "#######################................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            ".......................................#",
            "########################################",
            };

      static const char *mk4_xpm[] = {
            "40 13 2 1",
            "# c #ff0000",
            ". c none",
            "........................................",
            "........................................",
            "........................................",
            "#######################.................",
            "########################................",
            "########################................",
            "########################................",
            "########################................",
            "########################................",
            "#######################.................",
            "........................................",
            "........................................",
            "........................................",
            };

      if (octave == 0) {
            octave = new QPixmap(oct_xpm);
            mk1    = new QPixmap(mk1_xpm);
            mk2    = new QPixmap(mk2_xpm);
            mk3    = new QPixmap(mk3_xpm);
            mk4    = new QPixmap(mk4_xpm);
            }
      yRange = keyHeight * 75;
      }

//---------------------------------------------------------
//   pitch2y
//	y = 0 == origin of rCanvasA
//---------------------------------------------------------

int TimeCanvas::pitch2y(int pitch) const
      {
      int y;
      if (type == TIME_CANVAS_DRUMEDIT)
            y = pitch * drumHeight;
      else {
      	static int tt[] = {
	            12, 19, 25, 32, 38, 51, 58, 64, 71, 77, 84, 90
      	      };
	      y = (75 * keyHeight) - (tt[pitch % 12] + (7 * keyHeight) * (pitch / 12));
      	if (y < 0)
	            y = 0;
            }
      return lrint(y - wpos.y() / _ymag);
      }

//---------------------------------------------------------
//   y2pitch
//	y = 0 == origin of rCanvasA
//---------------------------------------------------------

int TimeCanvas::y2pitch(int y) const
      {
      y = lrint((y + wpos.y()) / _ymag);
      int pitch;
      if (type == TIME_CANVAS_DRUMEDIT)
            pitch = y / drumHeight;
      else {
      	const int total = (10 * 7 + 5) * keyHeight;       // 75 Ganztonschritte
            y = total - y;
            int oct = (y / (7 * keyHeight)) * 12;
            char kt[] = {
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  1, 1, 1, 1, 1, 1, 1,
                  2, 2, 2, 2, 2, 2,
                  3, 3, 3, 3, 3, 3, 3,
                  4, 4, 4, 4, 4, 4, 4, 4, 4,
                  5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                  6, 6, 6, 6, 6, 6, 6,
                  7, 7, 7, 7, 7, 7,
                  8, 8, 8, 8, 8, 8, 8,
                  9, 9, 9, 9, 9, 9,
                  10, 10, 10, 10, 10, 10, 10,
                  11, 11, 11, 11, 11, 11, 11, 11, 11, 11
                  };
            pitch = kt[y % 91] + oct;
            if (pitch < 0 || pitch > 127)
                  pitch = -1;
            }
      return pitch;
      }

//---------------------------------------------------------
//   paintPiano
//---------------------------------------------------------

void TimeCanvas::paintPiano(QPainter& p, QRect r)
      {
      int   d    = int(_ymag)+1;
      qreal x    = qreal(r.x());
      qreal y    = (r.y()-rulerHeight-d) / _ymag;
      if (y < 0.0)
            y = 0.0;
      qreal h = (r.height()+d) / _ymag;
      QPointF offset(x, wpos.y() / _ymag + keyHeight * 2 + y);

      p.translate(0.0, qreal(rulerHeight));
      p.scale(1.0, _ymag);
      p.drawTiledPixmap(QRectF(x, y, qreal(r.width()), h), *octave, offset);

      if (curPitch != -1) {
            int y = pitch2y(curPitch);
            QPixmap* pm;
            switch(curPitch % 12) {
                  case 0:
                  case 5:
                        pm = mk3;
                        break;
                  case 2:
                  case 7:
                  case 9:
                        pm = mk2;
                        break;
                  case 4:
                  case 11:
                        pm = mk1;
                        break;
                  default:
                        pm = mk4;
                        break;
                  }
            p.drawPixmap(0, y, *pm);
            }
      p.resetMatrix();
      }

//---------------------------------------------------------
//   paintPianorollHorizontalGrid
//---------------------------------------------------------

void TimeCanvas::paintPianorollHorizontalGrid(QPainter& p, QRect r)
      {
      qreal offset = rulerHeight - wpos.y();
      qreal kh = keyHeight * _ymag;

      int x1 = r.x();
      int x2 = x1 + r.width();
      if (x1 < pianoWidth)
            x1 = pianoWidth;
      qreal y  = kh + offset;
      qreal y1 = r.y() - _ymag;
      qreal y2 = y1 + r.height() + _ymag;
      for (int key = 1; key < 75; ++key, y += kh) {
            if (y < y1)
                  continue;
            if (y > y2)
                  break;
            switch (key % 7) {
                  case 2:
                  case 5:
                        p.setPen(QPen(Qt::lightGray));
                        break;
                  default:
                        p.setPen(QPen(Qt::gray));
                        break;
                  }
            p.drawLine(QLineF(x1, y, x2, y));
            }
      }

//---------------------------------------------------------
//   paintDrumeditHorizontalGrid
//---------------------------------------------------------

void TimeCanvas::paintDrumeditHorizontalGrid(QPainter& p, QRect r)
      {
      int offset = rulerHeight - wpos.y();

      p.setPen(QPen(Qt::lightGray));
      int x1 = r.x();
      int x2 = x1 + r.width();
      if (x1 < drumWidth)
            x1 = drumWidth;

      p.setPen(QPen(Qt::lightGray));

      int y  = offset;
      int y1 = r.y();
      int y2 = y1 + r.height();
      for (int i = 0; i < 128; ++i, y += drumHeight) {
            if (y < y1)
                  continue;
            if (y > y2)
                  break;
            p.drawLine(QLine(x1, y, x2, y));
            }
      }

//---------------------------------------------------------
//   addCtrlClicked
//---------------------------------------------------------

void TimeCanvas::addCtrlClicked()
      {
      addController();
      }

//---------------------------------------------------------
//   updateGeometry
//---------------------------------------------------------

void TimeCanvas::updateGeometry()
      {
      int wh = _widget->height();
      int ww = _widget->width();
      if (wh < ctrlHeight)
            ctrlHeight = wh;

      int x1 = 0;
      if (type == TIME_CANVAS_PIANOROLL)
            x1 = pianoWidth;
      else if (type == TIME_CANVAS_DRUMEDIT)
            x1 = drumWidth;
      else if (type == TIME_CANVAS_WAVEEDIT)
            x1 = waveWidth;
      int y2 = wh - ctrlHeight;

      rPanelA.setRect(0, rulerHeight, x1, wh - rulerHeight - ctrlHeight);
      rPanelB.setRect(0, y2,          x1, ctrlHeight);

      int cw = ww - x1;
      rRuler.setRect(  x1, 0,           cw, rulerHeight);
      rCanvasA.setRect(x1, rulerHeight, cw, wh - rulerHeight - ctrlHeight);
      rCanvasB.setRect(x1, y2,          cw, ctrlHeight);

      rButton.setRect(0, 0, rCanvasA.x(), rPanelA.y());

      if (yRange > 0 && _yFit) {
            _ymagMin = double(rCanvasA.height()) / double(yRange);
            if (_ymag < _ymagMin)
                  _ymag = _ymagMin;
            if (vmag)
                  vmag->setValue(lrint((_ymag-_ymagMin)*100.0/(_ymagMax-_ymagMin)));
            }
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void TimeCanvas::setTool(int t)
      {
      if (_tool == Tool(t))
            return;
      _tool = Tool(t);
      emit toolChanged(_tool);
      setCursor();
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void TimeCanvas::setCursor()
      {
      switch(_tool) {
            case PencilTool:
                  widget()->setCursor(QCursor(QPixmap(":/xpm/pencil.xpm"), 4, 15));
                  break;
            case RubberTool:
                  widget()->setCursor(QCursor(QPixmap(":/xpm/delete.xpm"), 4, 15));
                  break;
            case GlueTool:
                  widget()->setCursor(QCursor(QPixmap(":/xpm/glue.xpm"), 4, 15));
                  break;
            case CutTool:
                  widget()->setCursor(QCursor(QPixmap(":/xpm/cut.xpm"), 4, 15));
                  break;
            case MuteTool:
                  widget()->setCursor(QCursor(QPixmap(":/xpm/editmute.xmp"), 4, 15));
                  break;
            default:
                  widget()->setCursor(QCursor(Qt::ArrowCursor));
                  break;
            }
      }

//---------------------------------------------------------
//   setCanvasBackground
//---------------------------------------------------------

void TimeCanvas::setCanvasBackground(const QColor& color)
      {
	canvasBackgroundPixmap = QPixmap();
      canvasBackgroundColor = color;
      widget()->update();
      }

//---------------------------------------------------------
//   setCanvasBackground
//---------------------------------------------------------

void TimeCanvas::setCanvasBackground(const QPixmap& pm)
      {
      canvasBackgroundPixmap = pm;
      widget()->update();
      }

//---------------------------------------------------------
//   setYMagRange
//---------------------------------------------------------

void TimeCanvas::setYMagRange(double min, double max)
      {
      _ymagMin = min;
      _ymagMax = max;
      if (vmag)
            vmag->setValue(lrint((_ymag-_ymagMin)*100.0/(_ymagMax-_ymagMin)));
      }

//---------------------------------------------------------
//   setVSize
//---------------------------------------------------------

void TimeCanvas::setVSize(int val)
      {
      if (yRange == val)
            return;
      yRange = val;
      if (_yFit) {
            _ymagMin = double(rCanvasA.height()) / double(yRange);
            if (_ymag < _ymagMin)
                  _ymag = _ymagMin;
            if (vmag)
                  vmag->setValue(lrint((_ymag-_ymagMin)*100.0/(_ymagMax-_ymagMin)));
            }
      updateScrollBars();
      }

//---------------------------------------------------------
//   s2xmag
//	nonlinear xmag behaviour, feels better
//---------------------------------------------------------

double TimeCanvas::s2xmag(int val)
	{
      val = 100 - val;
	double f = 1.0 - log10(val * val + 1) * 0.25;
      return (_xmagMax - _xmagMin) * f + _xmagMin;
      }

//---------------------------------------------------------
//   xmag2s
//---------------------------------------------------------

int TimeCanvas::xmag2s(double m)
	{
      m -= _xmagMin;
      m /= (_xmagMax - _xmagMin);
      double val = sqrt(exp10((1.0 - m) * 4.0)-1.0);
      return lrint(100.0 - val);
      }

//---------------------------------------------------------
//   setPart
//---------------------------------------------------------

void TimeCanvas::setPart(const AL::Pos& p1, const AL::Pos& p2)
	{
      partPos1 = p1;
      partPos2 = p2;
      widget()->update();
      }

//---------------------------------------------------------
//   setFont1
//---------------------------------------------------------

void TimeCanvas::setFont1(const QFont& f)
      {
      _font1 = f;
      printf("TimeCanvas::setFont1\n");
      }

//---------------------------------------------------------
//   setFont2
//---------------------------------------------------------

void TimeCanvas::setFont2(const QFont& f)
      {
      _font2 = f;
      }

//---------------------------------------------------------
//   setFont3
//---------------------------------------------------------

void TimeCanvas::setFont3(const QFont& f)
      {
      _font3 = f;
      }

