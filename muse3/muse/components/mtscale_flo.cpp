//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale_flo.cpp,v 1.8.2.7 2011/05/19 04:14:01 flo Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <limits.h>

#include "mtscale_flo.h"
#include "song.h"
#include "app.h"
#include "icons.h"
#include "gconfig.h"
#include "scoreedit.h"
#include "globals.h"

// NOTE: To cure circular dependencies these includes are at the bottom.
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   MTScale
//    Midi Time Scale
//---------------------------------------------------------

MTScaleFlo::MTScaleFlo(ScoreCanvas* parent_editor, QWidget* parent_widget)
   : View(parent_widget, 1, 1)
      {
      setToolTip(tr("Bar scale"));
			pos[0] = MusEGlobal::song->cpos();
			pos[1] = MusEGlobal::song->lpos();
			pos[2] = MusEGlobal::song->rpos();
      xpos=0;
      xoffset=0;
      
      button = Qt::NoButton;
      setMouseTracking(true);
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::song, SIGNAL(markerChanged(int)), SLOT(redraw()));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));

      parent=parent_editor;
	
      setFixedHeight(28);
      setBg(MusEGlobal::config.rulerBg);
      }

void MTScaleFlo::configChanged()
      {
      setBg(MusEGlobal::config.rulerBg);


      }
//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MTScaleFlo::songChanged(MusECore::SongChangedStruct_t type)
      {
      if (type & (SC_SIG|SC_TEMPO|SC_MARKERS_REBUILT|SC_MARKER_INSERTED|SC_MARKER_REMOVED|SC_MARKER_MODIFIED))
            redraw();
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void MTScaleFlo::setPos(int idx, unsigned val, bool)
      {
      if ((val == INT_MAX) || (val == pos[idx]))
            return;

      int opos = parent->tick_to_x(pos[idx] == INT_MAX ? val : pos[idx]) + xoffset - xpos;

      pos[idx] = val;
      
      if (isVisible()) {

            int tval   = parent->tick_to_x(val) + xoffset - xpos;
            int x = -9;
            int w = 18;

            if (tval < 0) { // tval<0 occurs whenever the window is scrolled left, so I switched to signed int (ml)
                  redraw();
                  }
            else if (opos > tval) { 
                  w += opos - tval;
                  x += tval;
                  }
            else {
                  w += tval - opos;
                  x += opos;
                  }
            redraw(QRect(x, 0, w, height()));
            }
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void MTScaleFlo::mousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      mouseMoveEvent(event);
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void MTScaleFlo::mouseReleaseEvent(QMouseEvent*)
      {
      button = Qt::NoButton;
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void MTScaleFlo::mouseMoveEvent(QMouseEvent* event)
      {
      if (event->modifiers() & Qt::ShiftModifier )
            setCursor(QCursor(Qt::PointingHandCursor));
      else
            setCursor(QCursor(Qt::ArrowCursor));
      
      int x = event->x();
      if (x<0) x=0;
      int tick = parent->x_to_tick(x-xoffset+xpos);
      if (tick<0) tick=0;
      tick = MusEGlobal::sigmap.raster(tick, parent->quant_ticks());

      MusECore::Song::POSTYPE posType;

      switch (button) {
      case Qt::LeftButton:
          if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_LEFT_CTRL_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::LPOS;
          else
              posType = MusECore::Song::CPOS;
          break;
      case Qt::MidButton:
          posType = MusECore::Song::LPOS;
          break;
      case Qt::RightButton:
          if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_LEFT_CTRL_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::RPOS;
          else if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_RIGHT_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::LPOS;
          else
              posType = MusECore::Song::RPOS;
          break;
      default:
          return; // if no button is pressed the function returns here
      }
      MusECore::Pos p(tick, true);
      
      if(posType == MusECore::Song::CPOS && (event->modifiers() & Qt::ShiftModifier )) {        // If shift +LMB we add a marker
            const MusECore::iMarker alreadyExists = MusEGlobal::song->getMarkerAt(tick);
            if (alreadyExists == MusEGlobal::song->marker()->end())
                  MusEGlobal::song->addMarker(QString(""), tick, false);         
            }
      else if (posType == MusECore::Song::RPOS && (event->modifiers() & Qt::ShiftModifier )) {  // If shift +RMB we remove a marker
            const MusECore::iMarker toRemove = MusEGlobal::song->getMarkerAt(tick);
            if (toRemove != MusEGlobal::song->marker()->end())
              MusEGlobal::song->removeMarker(toRemove->second);
            else
              fprintf(stderr, "No marker to remove\n");
            }
      else
            MusEGlobal::song->setPos(posType, p);                             // all other cases: relocating one of the locators
      }



//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void MTScaleFlo::draw(QPainter& p, const QRect& r, const QRegion&)
      {
      int x = r.x();
      int w = r.width();
      
      x -= 20;
      w += 40;    // wg. Text

      //---------------------------------------------------
      //    draw Marker
      //---------------------------------------------------

      int y = 12;
      p.setPen(MusEGlobal::config.rulerFg);
      p.setFont(MusEGlobal::config.fonts[5]);
      p.drawLine(r.x(), y+1, r.x() + r.width(), y+1);
      QRect tr(r);
      tr.setHeight(12);
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            
            int xp = parent->tick_to_x(m->second.tick()) + xoffset - xpos;
            if (xp > x+w)
                  break;
            int xe = r.x() + r.width();
	    MusECore::iMarker mm = m;
            ++mm;
            if (mm != marker->end())
                    xe = parent->tick_to_x(mm->first) + xoffset - xpos;
            
            QRect tr(xp, 0, xe-xp, 13);
                    
            QRect wr = r.intersected(tr);
            if(!wr.isEmpty()) 
            {        
              if (m->second.current()) 
                    p.fillRect(wr, MusEGlobal::config.rulerCurrent);
              
              int x2;
              if (mm != marker->end())
										x2 = parent->tick_to_x(mm->first) + xoffset - xpos;
              else
                    x2 = xp+200;
              
              if(xp >= -32)
                p.drawPixmap(xp, 0, *flagIconS);
                
              if(xp >= -1023)
              {
                QRect r = QRect(xp+10, 0, x2-xp, 12);
                p.setPen(MusEGlobal::config.rulerFg);
                p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, m->second.name());
              }  
              
              if(xp >= 0)
              {
                p.setPen(Qt::green);
                p.drawLine(xp, y, xp, height());
              }  
            }  
      }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      int h = height()-12;

			for (int i = 0; i < 3; ++i) {
						int xp = parent->tick_to_x(pos[i]) + xoffset - xpos;
						if (xp >= x && xp < x+w) {
									QPixmap* pm = markIcon[i];
									p.drawPixmap(xp - pm->width()/2, y-1, *pm);
									}
						}

      
      //---------------------------------------------------
      //    draw beats
      //---------------------------------------------------


      p.setPen(MusEGlobal::config.rulerFg);

      unsigned ctick;
      int bar1, bar2, beat;
      unsigned tick;

			ctick = parent->x_to_tick(x - xoffset + xpos);
			MusEGlobal::sigmap.tickValues(ctick, &bar1, &beat, &tick);
			MusEGlobal::sigmap.tickValues(parent->x_to_tick(x+w - xoffset + xpos), &bar2, &beat, &tick);


      int stick = MusEGlobal::sigmap.bar2tick(bar1, 0, 0);
      int ntick;
      for (int bar = bar1; bar <= bar2; bar++, stick = ntick) {
            ntick     = MusEGlobal::sigmap.bar2tick(bar+1, 0, 0);
            int tpix = parent->delta_tick_to_delta_x(ntick - stick);
            if (tpix < 64) {
                  // donï¿½t show beats if measure is this small
                  int n = 1;
                  if (tpix < 32)
                        n = 2;
                  if (tpix <= 16)
                        n = 4;
                  if (tpix < 8)
                        n = 8;
                  if (tpix <= 4)
                        n = 16;
                  if (tpix <= 2)
                        n = 32;
                  if (bar % n)
                        continue;
                  p.setFont(MusEGlobal::config.fonts[3]);
                  int x = parent->tick_to_x(stick) + xoffset - xpos;
                  QString s;
                  s.setNum(bar + 1);
                  p.drawLine(x, y+1, x, y+1+h);
                  QRect r = QRect(x+2, y, 1000, h);
                  p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                  }
            else {
                  int z, n;
                  MusEGlobal::sigmap.timesig(stick, z, n);
                  for (int beat = 0; beat < z; beat++) {
                        int xp = parent->tick_to_x(MusEGlobal::sigmap.bar2tick(bar, beat, 0)) + xoffset - xpos;
                        QString s;
                        QRect r(xp+2, y, 1000, h);
                        int y1;
                        int num;
                        if (beat == 0) {
                              num = bar + 1;
                              y1  = y + 1;
                              p.setFont(MusEGlobal::config.fonts[3]);
                              }
                        else {
                              num = beat + 1;
                              y1  = y + 7;
                              p.setFont(MusEGlobal::config.fonts[4]);
                              r.setY(y+3);
                              }
                        s.setNum(num);
                        p.drawLine(xp, y1, xp, y+1+h);
                        p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                        }
                  }
            }
      }

void MTScaleFlo::set_xpos(int pos)
{
	xpos=pos;
	redraw();
}

void MTScaleFlo::set_xoffset(int o)
{
	xoffset=o;
	redraw();
}

void MTScaleFlo::pos_add_changed()
{
	redraw();
}

} // namespace MusEGui
