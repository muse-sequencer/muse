//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale.cpp,v 1.8.2.7 2009/05/03 04:14:01 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <values.h>

#include <QMouseEvent>
#include <QPainter>

#include "pcscale.h"
#include "song.h"
#include "icons.h"
#include "gconfig.h"
#include "prcanvas.h"

//---------------------------------------------------------
//   PCScale
//    Midi Time Scale
//---------------------------------------------------------

PCScale::PCScale(int* r, QWidget* parent, PianoRoll* editor, int xs, bool _mode)
   : View(parent, xs, 1)
      {
	  currentEditor = editor;
      waveMode = _mode;
      setToolTip(tr("bar pcscale"));
      barLocator = false;
      raster = r;
      if (waveMode) {
            pos[0] = tempomap.tick2frame(song->cpos());
            pos[1] = tempomap.tick2frame(song->lpos());
            pos[2] = tempomap.tick2frame(song->rpos());
      }
      else {
            pos[0] = song->cpos();
            pos[1] = song->lpos();
            pos[2] = song->rpos();
      }
      pos[3] = MAXINT;            // do not show
      button = Qt::NoButton;
      setMouseTracking(true);
      connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      //connect(song, SIGNAL(markerChanged(int)), SLOT(redraw()));
	
      setFixedHeight(14);
      setBg(QColor(110, 141, 152));
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void PCScale::songChanged(int type)
{
      if (type & (SC_SIG|SC_TEMPO)) {
           if ((type & SC_TEMPO) && waveMode) {
                  pos[0] = tempomap.tick2frame(song->cpos());
                  pos[1] = tempomap.tick2frame(song->lpos());
                  pos[2] = tempomap.tick2frame(song->rpos());
           }
           redraw();
      }
      redraw();
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PCScale::setPos(int idx, unsigned val, bool)
{
      if (val == MAXINT) {
            if (idx == 3) {
                  pos[3] = MAXINT;
                  redraw(QRect(0, 0, width(), height()));
            }
            return;
      }
      if (waveMode)
            val = tempomap.tick2frame(val);
      if (val == pos[idx])
            return;
      //unsigned opos = mapx(pos[idx] == MAXINT ? val : pos[idx]);
      int opos = mapx(pos[idx] == MAXINT ? val : pos[idx]);
      pos[idx] = val;
      if (!isVisible())
            return;

      int tval   = mapx(val);
      int x = -9;
      int w = 18;

      if (tval < 0) { // tval<0 occurs whenever the window is scrolled left, so I switched to signed int (ml)
            //printf("PCScale::setPos - idx:%d val:%d tval:%d opos:%d w:%d h:%d\n", idx, val, tval, opos, width(), height());
      
            redraw(QRect(0,0,width(),height()));
            return;
      }
      //if (opos > (unsigned int) tval) {	//prevent compiler warning: comparison signed/unsigned
      if (opos > tval) { 
            w += opos - tval;
            x += tval;
      }
      else {
            w += tval - opos;
            x += opos;
      }
      //printf("PCScale::setPos idx:%d val:%d tval:%d opos:%d x:%d w:%d h:%d\n", idx, val, tval, opos, x, w, height());
      
      redraw(QRect(x, 0, w, height()));
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void PCScale::viewMousePressEvent(QMouseEvent* event)
{
      button = event->button();
      viewMouseMoveEvent(event);
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PCScale::viewMouseReleaseEvent(QMouseEvent*)
{
      button = Qt::NoButton;
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void PCScale::viewMouseMoveEvent(QMouseEvent* event)
{
      if (event->modifiers() & Qt::ShiftModifier )
            setCursor(QCursor(Qt::PointingHandCursor));
      else
            setCursor(QCursor(Qt::ArrowCursor));
      
      int x = event->x();
      x = AL::sigmap.raster(x, *raster);
      if (x < 0)
            x = 0;
      //printf("PCScale::viewMouseMoveEvent\n");  
      int i;
      switch (button) {
            case Qt::LeftButton:
                  i = 0;
                  break;
            case Qt::MidButton:
                  i = 1;
                  break;
            case Qt::RightButton:
                  i = 2;
                  break;
            default:
                  return; // if no button is pressed the function returns here
      }
    Pos p(x, true);
	if (waveMode)
	{
		song->setPos(i, p);
		return;
	}
      
	if(i== 0 && (event->modifiers() & Qt::ShiftModifier )) {        // If shift +LMB we add a marker 
		//Add program change here
		song->setPos(i, p);                             // all other cases: relocating one of the locators
		emit selectInstrument();
		emit addProgramChange();
	}
	else if (i== 2 && (event->modifiers() & Qt::ShiftModifier )) {  // If shift +RMB we remove a marker 
		//Delete Program change here
		//song->setPos(i, p);
		Track* track = song->findTrack(currentEditor->curCanvasPart());
		PartList* parts = track->parts();
		for (iPart p = parts->begin(); p != parts->end(); ++p) 
		{
			Part* mprt = p->second;
			EventList* eventList = mprt->events();//m->second.events();
			for(iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
			{
				//Get event type.
				Event pcevt = evt->second;
				printf("Found events %d \n", pcevt.type());
				if(!pcevt.isNote())
				{
					printf("Found none Note events of type: %d with dataA: %d\n", pcevt.type(), pcevt.dataA());
					if(pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
					{
						printf("Found Program Change event type\n");
						printf("Pos x: %d\n", x);
						int xp = pcevt.tick()+mprt->tick();
						printf("Event x: %d\n", xp);
						if(xp >= x && xp <= (x+50))
						{
							printf("Found Program Change to delete at: %d\n", x);
							//audio->msgDeleteEvent(pcevt, mprt, false, true, true);
							pcevt.setSelected(true);
							//currentEditor->deleteSelectedProgramChange();
							song->deleteEvent(pcevt, mprt);
							//currentEditor->deleteEvent(pcevt, mprt);
						}
					}
				}
			}
		}
	}                                                        
	else
		song->setPos(i, p);                             // all other cases: relocating one of the locators
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PCScale::leaveEvent(QEvent*)
{
      //emit timeChanged(MAXINT);
}

void PCScale::setEditor(PianoRoll* editor)
{
	currentEditor = editor;
}

void PCScale::updateProgram()
{
		redraw();
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PCScale::pdraw(QPainter& p, const QRect& r)
{
	if(waveMode) 
		return;
	int x = r.x();
	int w = r.width();
	
	x -= 20;
	w += 40;    // wg. Text
	
	//---------------------------------------------------
	//    draw Flag
	//---------------------------------------------------
	
	int y = 12;
	p.setPen(Qt::black);
	p.setFont(config.fonts[4]);
	p.drawLine(r.x(), y+1, r.x() + r.width(), y+1);
	QRect tr(r);
	tr.setHeight(12);
	Track* track = song->findTrack(currentEditor->curCanvasPart());
	PartList* parts = track->parts();
	for (iPart m = parts->begin(); m != parts->end(); ++m) 
	{
		Part* mprt = m->second;
		EventList* eventList = mprt->events();
		for(iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
		{
			//Get event type.
			Event pcevt = evt->second;
			if(!pcevt.isNote())
			{
				if(pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
				{
					int xp = mapx(pcevt.tick()+mprt->tick());
					if (xp > x+w)
					{
						//printf("Its dying from greater than bar size\n");
						break;
					}
					int xe = r.x() + r.width();
					iEvent mm = evt;
					++mm;
					
					QRect tr(xp, 0, xe-xp, 13);
					        
					QRect wr = r.intersect(tr);
					if(!wr.isEmpty()) 
					{
						int x2;
						if (mm != eventList->end())
						{
							x2 = mapx(pcevt.tick() + mprt->tick());
						}      
						else
							x2 = xp+200;
						
						//printf("PCScale::pdraw marker %s xp:%d y:%d h:%d r.x:%d r.w:%d\n", "Test Debug", xp, height(), y, r.x(), r.width());
						
						// Must be reasonable about very low negative x values! With long songs > 15min
						//  and with high horizontal magnification, 'ghost' drawings appeared,
						//  apparently the result of truncation later (xp = -65006 caused ghosting
						//  at bar 245 with magnification at max.), even with correct clipping region
						//  applied to painter in View::paint(). Tim.  Apr 5 2009 
						// Quote: "Warning: Note that QPainter does not attempt to work around 
						//  coordinate limitations in the underlying window system. Some platforms may 
						//  behave incorrectly with coordinates as small as +/-4000."
						if(xp >= -32)
							p.drawPixmap(xp, 0, *flagIconS);
						  
					//	if(xp >= -1023)
					//	{
					//		QRect r = QRect(xp+10, 0, x2-xp, 12);
					//		p.setPen(Qt::black);
					//		//Use the program change info as name
					//		p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, "Test"/*pcevt.name()*/);
					//	}  
						
						if(xp >= 0)
						{
							p.setPen(Qt::green);
							p.drawLine(xp, y, xp, height());
						}  
					}//END if(wr.isEmpty)
				}//END if(CTRL_PROGRAM)
			}//END if(!isNote)
		}
	}
}

