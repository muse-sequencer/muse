//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/bigtime.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
//#include <stdio.h>
#include <limits.h>
#include <cmath>

#include <QCheckBox>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>

#include "sync.h"
#include "sig.h"
#include "globals.h"
#include "bigtime.h"
#include "app.h"
#include "gconfig.h"
#include "audio.h"
#include "song.h"

namespace MusEGlobal {
extern int mtcType;
}

namespace MusEGui {

//
// the bigtime widget
// display is split into several parts to avoid flickering.
//

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

BigTime::BigTime(QWidget* parent)
   : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)  // Possibly also Qt::X11BypassWindowManagerHint
{
// REMOVE Tim. clip. Removed.
//   _curPos = 0;
  //_curPos.setType(MusECore::Pos::FRAMES);
  _formatted = true;
  dwin = new QWidget(this, Qt::WindowStaysOnTopHint);  // Possibly also Qt::X11BypassWindowManagerHint
  dwin->setObjectName("bigtime-dwin");
  dwin->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  dwin->setStyleSheet("font-size:10px; font-family:'Courier'; ");   // Tim p4.0.8

  fmtButton = new QCheckBox(QString(""), this);
  fmtButton->resize(18,18);
  fmtButton->setChecked(true);
  fmtButton->setToolTip(tr("format display"));
  fmtButton->setFocusPolicy(Qt::NoFocus);
  barLabel   = new QLabel(dwin);
  beatLabel  = new QLabel(dwin);
  tickLabel  = new QLabel(dwin);
  //hourLabel  = new QLabel(dwin);
  minLabel   = new QLabel(dwin);
  secLabel   = new QLabel(dwin);
  frameLabel = new QLabel(dwin);
  subFrameLabel  = new QLabel(dwin);
  sep1       = new QLabel(QString("."), dwin);
  sep2       = new QLabel(QString("."), dwin);
  sep3       = new QLabel(QString(":"), dwin);
  sep4       = new QLabel(QString(":"), dwin);
  sep5       = new QLabel(QString(":"), dwin);
  absTickLabel = new QLabel(dwin);
  absFrameLabel = new QLabel(dwin);
  barLabel->setToolTip(tr("bar"));
  beatLabel->setToolTip(tr("beat"));
  tickLabel->setToolTip(tr("tick"));
  //hourLabel->setToolTip(tr("hour"));
  minLabel->setToolTip(tr("minute"));
  secLabel->setToolTip(tr("second"));
  frameLabel->setToolTip(tr("frame"));
  subFrameLabel->setToolTip(tr("subframe"));
  absTickLabel->setToolTip(tr("tick"));
  absFrameLabel->setToolTip(tr("frame"));
  fmtButtonToggled(true);
  connect(fmtButton, SIGNAL(toggled(bool)), SLOT(fmtButtonToggled(bool)));
  //oldbar = oldbeat = oldtick = oldhour = oldmin = oldsec = oldframe = -1;
  oldbar = oldbeat = oldtick = oldmin = oldsec = oldframe = oldsubframe = -1;
  oldAbsTick = oldAbsFrame = -1;
  setString(INT_MAX);
  metronome = new VerticalMetronomeWidget(dwin);

  configChanged();

  setWindowTitle(tr("MusE: Bigtime"));
}

//---------------------------------------------------------
//   fmtButtonToggled
//---------------------------------------------------------

void BigTime::fmtButtonToggled(bool v)
{
    if(v)
    {
      _formatted = true;
      
      barLabel->setEnabled(true);   
      beatLabel->setEnabled(true);  
      tickLabel->setEnabled(true);
      //hourLabel->setEnabled(true);
      minLabel->setEnabled(true);
      secLabel->setEnabled(true);
      frameLabel->setEnabled(true);
      subFrameLabel->setEnabled(true);
      sep1->setEnabled(true);
      sep2->setEnabled(true);
      sep3->setEnabled(true);
      sep4->setEnabled(true);
      sep5->setEnabled(true);
      absTickLabel->setEnabled(false);
      absFrameLabel->setEnabled(false);
      
      barLabel->show();   
      beatLabel->show();  
      tickLabel->show();
      //hourLabel->show();
      minLabel->show();
      secLabel->show();
      frameLabel->show();
      subFrameLabel->show();
      sep1->show();
      sep2->show();
      sep3->show();
      sep4->show();
      sep5->show();
      absTickLabel->hide();
      absFrameLabel->hide();
    }
    else
    {
      _formatted = false;
      
      barLabel->setEnabled(false);   
      beatLabel->setEnabled(false);  
      tickLabel->setEnabled(false);
      //hourLabel->setEnabled(false);
      minLabel->setEnabled(false);
      secLabel->setEnabled(false);
      frameLabel->setEnabled(false);
      subFrameLabel->setEnabled(false);
      sep1->setEnabled(false);
      sep2->setEnabled(false);
      sep3->setEnabled(false);
      sep4->setEnabled(false);
      sep5->setEnabled(false);
      absTickLabel->setEnabled(true);
      absFrameLabel->setEnabled(true);
      
      barLabel->hide();   
      beatLabel->hide();  
      tickLabel->hide();
      //hourLabel->hide();
      minLabel->hide();
      secLabel->hide();
      frameLabel->hide();
      subFrameLabel->hide();
      sep1->hide();
      sep2->hide();
      sep3->hide();
      sep4->hide();
      sep5->hide();
      absTickLabel->show();
      absFrameLabel->show();
    }
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void BigTime::configChanged()
      {
      setBgColor(MusEGlobal::config.bigTimeBackgroundColor);
      setFgColor(MusEGlobal::config.bigTimeForegroundColor);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void BigTime::songChanged(MusECore::SongChangedStruct_t flags)
      {
// REMOVE Tim. clip. Changed.
      if (flags._flags & (SC_MASTER | SC_TEMPO | SC_SIG))

      // Catch SC_SIG in case BBT needs to update.
      // Note tempo and master changes are handled by the setPos() slot.
      //if (flags._flags & (SC_SIG))
      {
        // REMOVE Tim. clip. Added.
//         // If the cached position is in frames, the value is invalid.
//         // We must use the current tick and update the frame in order
//         //  to keep the current tick the same, ie. we must respect the current
//         //  visual tick-based cursor positions.
//         // If we did it the other way round ie. try to keep the current frame
//         //  and calculate the new tick from it, then the tick-based cursors
//         //  would jump around upon every tempo adjustment.
//         // So we must update the tick value of the cached position.
//         // Note that it will quantize the frame to whatever the tick is,
//         //  ie. it is not possible to end up on an in-between frame
//         //  after a tempo adjustment.
//         // TODO Note that we should tell the transport to relocate whenever
//         //  tempo changes so that the resulting frame matches the tick.
//         // Or else the other way around - keep the frame and relocate
//         //  the tick-based cursors - but that would make them jump.
//         _curPos.setTick(MusEGlobal::song->cPos().tick());
        
        updateValue();
      }
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void BigTime::closeEvent(QCloseEvent *ev)
      {
      emit closed();
      QWidget::closeEvent(ev);
      }

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   setString
// //---------------------------------------------------------
// 
// bool BigTime::setString(unsigned v)
//       {
//       if (v == INT_MAX) {
//         barLabel->setText(QString("----"));
//         beatLabel->setText(QString("--"));
//         tickLabel->setText(QString("---"));
//         //hourLabel->setText(QString("--"));
//         //minLabel->setText(QString("--"));
//         minLabel->setText(QString("---"));
//         secLabel->setText(QString("--"));
//         frameLabel->setText(QString("--"));
//         subFrameLabel->setText(QString("--"));
// 
//         absTickLabel->setText(QString("----------"));
//         absFrameLabel->setText(QString("----------"));
//         oldAbsTick = oldAbsFrame = -1;
//         //oldbar = oldbeat = oldtick = oldhour = oldmin = oldsec = oldframe = -1;
//         oldbar = oldbeat = oldtick = oldmin = oldsec = oldframe = oldsubframe = -1;
//         return true;
//       }
// 
//       // Quick fix: Not much to do but ignore the supplied tick: We need the exact frame here.
//       unsigned absFrame = MusEGlobal::audio->pos().frame();
//       
//       int bar, beat;
//       unsigned tick;
//       MusEGlobal::sigmap.tickValues(v, &bar, &beat, &tick);
//       double time = double(absFrame)/double(MusEGlobal::sampleRate);
//       //int hour    = int(time) / 3600;
//       //int min     = (int(time) / 60) % 60;
//       int min     = int(time) / 60;
//       int sec     = int(time) % 60;
//       double rest = time - (min * 60 + sec);
//       switch(MusEGlobal::mtcType) {
//           case 0:     // 24 frames sec
//                 rest *= 24;
//                 break;
//           case 1:     // 25
//                 rest *= 25;
//                 break;
//           case 2:     // 30 drop frame
//                 rest *= 30;
//                 break;
//           case 3:     // 30 non drop frame
//                 rest *= 30;
//                 break;
//       }
//       int frame = int(rest);
//       int subframe = int((rest-frame)*100);
// 
//       QString s;
// 
//       if(oldAbsTick != v) {
//         s = QString("%1").arg(v, 10, 10, QLatin1Char('0'));
//         absTickLabel->setText(s);
//         oldAbsTick = v;
//       }
//       if(oldAbsFrame != absFrame) {
//         s = QString("%1").arg(absFrame, 10, 10, QLatin1Char('0'));
//         absFrameLabel->setText(s);
//         oldAbsFrame = absFrame;
//       }
//       if(oldbar != bar) {
//         s = QString("%1").arg(bar + 1, 4, 10, QLatin1Char('0'));
// 	      barLabel->setText(s);
// 	      oldbar = bar;
//       }
//       if(oldbeat != beat) {
//         s = QString("%1").arg(beat + 1, 2, 10, QLatin1Char('0'));
// 	      beatLabel->setText(s);
// 	      oldbeat = beat;
//       }
// 
//       if(oldtick != tick) {
//         s = QString("%1").arg(tick, 3, 10, QLatin1Char('0'));
// 	      tickLabel->setText(s);
// 	      oldtick = tick;
//       }
// 
//       //if(oldhour != hour) {
//       //  s = QString("%1").arg(hour, 2, 10, QLatin1Char('0'));
// 	    //  hourLabel->setText(s);
// 	    //  oldhour = hour;
//       //}
// 
//       if(oldmin != min) {
//         s = QString("%1").arg(min, 3, 10, QLatin1Char('0'));
// 	      minLabel->setText(s);
// 	      oldmin = min;
//       }
// 
//       if(oldsec != sec) {
//         s = QString("%1").arg(sec, 2, 10, QLatin1Char('0'));
// 	      secLabel->setText(s);
// 	      oldsec = sec;
//       }
// 
//       if(oldframe != frame) {
//         s = QString("%1").arg(frame, 2, 10, QLatin1Char('0'));
// 	      frameLabel->setText(s);
// 	      oldframe = frame;
//       }
// 
//       if(oldsubframe != subframe) {
//         s = QString("%1").arg(subframe, 2, 10, QLatin1Char('0'));
//         subFrameLabel->setText(s);
//         oldsubframe = subframe;
//       }
// 
//       return false;
//       }

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool BigTime::setString(const MusECore::Pos v)
      {
      if ((v.type() == MusECore::Pos::TICKS && v.tick() >= MUSE_INVALID_POSITION) || 
          (v.type() == MusECore::Pos::FRAMES && v.frame() >= MUSE_INVALID_POSITION)) {
        barLabel->setText(QString("----"));
        beatLabel->setText(QString("--"));
        tickLabel->setText(QString("---"));
        //hourLabel->setText(QString("--"));
        //minLabel->setText(QString("--"));
        minLabel->setText(QString("---"));
        secLabel->setText(QString("--"));
        frameLabel->setText(QString("--"));
        subFrameLabel->setText(QString("--"));

        absTickLabel->setText(QString("----------"));
        absFrameLabel->setText(QString("----------"));
        oldAbsTick = oldAbsFrame = -1;
        //oldbar = oldbeat = oldtick = oldhour = oldmin = oldsec = oldframe = -1;
        oldbar = oldbeat = oldtick = oldmin = oldsec = oldframe = oldsubframe = -1;
        return true;
      }

      const unsigned int pos_frame = v.frame();
      const unsigned int pos_tick = v.tick();
      
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(pos_tick, &bar, &beat, &tick);
      
      
      int min, sec, frame, subframe;
      
      v.msf(&min, &sec, &frame, &subframe);
      
      QString s;

      if(oldAbsTick != pos_tick) {
        s = QString("%1").arg(pos_tick, 10, 10, QLatin1Char('0'));
        absTickLabel->setText(s);
        oldAbsTick = pos_tick;
      }
      if(oldAbsFrame != pos_frame) {
        s = QString("%1").arg(pos_frame, 10, 10, QLatin1Char('0'));
        absFrameLabel->setText(s);
        oldAbsFrame = pos_frame;
      }
      if(oldbar != bar) {
        s = QString("%1").arg(bar + 1, 4, 10, QLatin1Char('0'));
	      barLabel->setText(s);
	      oldbar = bar;
      }
      if(oldbeat != beat) {
        s = QString("%1").arg(beat + 1, 2, 10, QLatin1Char('0'));
	      beatLabel->setText(s);
	      oldbeat = beat;
      }

      if(oldtick != tick) {
        s = QString("%1").arg(tick, 3, 10, QLatin1Char('0'));
	      tickLabel->setText(s);
	      oldtick = tick;
      }

      //if(oldhour != hour) {
      //  s = QString("%1").arg(hour, 2, 10, QLatin1Char('0'));
	    //  hourLabel->setText(s);
	    //  oldhour = hour;
      //}

      if(oldmin != min) {
        s = QString("%1").arg(min, 3, 10, QLatin1Char('0'));
	      minLabel->setText(s);
	      oldmin = min;
      }

      if(oldsec != sec) {
        s = QString("%1").arg(sec, 2, 10, QLatin1Char('0'));
	      secLabel->setText(s);
	      oldsec = sec;
      }

      if(oldframe != frame) {
        s = QString("%1").arg(frame, 2, 10, QLatin1Char('0'));
	      frameLabel->setText(s);
	      oldframe = frame;
      }

      if(oldsubframe != subframe) {
        s = QString("%1").arg(subframe, 2, 10, QLatin1Char('0'));
        subFrameLabel->setText(s);
        oldsubframe = subframe;
      }

      return false;
      }

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   setPos
// //---------------------------------------------------------
// #define PI 3.14159265
// void BigTime::setPos(int idx, unsigned v, bool)
// {
//   if (idx == 0)
//   {
//     _curPos = v;
//     int calcV = v%(MusEGlobal::config.midiDivision*2);
//     double rangeAdjuster = PI/double(MusEGlobal::config.midiDivision);
//     metronome->setMetronome(sin(double(calcV)*rangeAdjuster));
//     //printf("calcV=%d rangeAdjuster %f metronomePosition=%f midiDivision=%d\n",v,rangeAdjuster, metronomePosition, MusEGlobal::config.midiDivision);
//     setString(v);
//   }
//   metronome->update();
// }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------
#define PI 3.14159265
void BigTime::setPos(int idx, const MusECore::Pos v, bool)
{
  if (idx == 0)
  {
    _curPos = v;
    //_curPos.setTickAndFrame(v);
    //_curPos.setPos(v);
//     int calcV = v.tick()%(MusEGlobal::config.midiDivision*2);
    int calcV = _curPos.tick()%(MusEGlobal::config.midiDivision*2);
    double rangeAdjuster = PI/double(MusEGlobal::config.midiDivision);
    metronome->setMetronome(sin(double(calcV)*rangeAdjuster));
    //printf("calcV=%d rangeAdjuster %f metronomePosition=%f midiDivision=%d\n",v,rangeAdjuster, metronomePosition, MusEGlobal::config.midiDivision);
    setString(v);
  }
  metronome->update();
}

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   updateValue
// //---------------------------------------------------------
// 
// void BigTime::updateValue()
// {
//   int calcV = _curPos%(MusEGlobal::config.midiDivision*2);
//   double rangeAdjuster = PI/double(MusEGlobal::config.midiDivision);
//   metronome->setMetronome(sin(double(calcV)*rangeAdjuster));
//   //printf("calcV=%d rangeAdjuster %f metronomePosition=%f midiDivision=%d\n",_curPos,rangeAdjuster, metronomePosition, MusEGlobal::config.midiDivision);
//   setString(_curPos);
//   update();
//   metronome->update();
// }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void BigTime::updateValue()
{
  int calcV = _curPos.tick()%(MusEGlobal::config.midiDivision*2);
  double rangeAdjuster = PI/double(MusEGlobal::config.midiDivision);
  metronome->setMetronome(sin(double(calcV)*rangeAdjuster));
  //printf("calcV=%d rangeAdjuster %f metronomePosition=%f midiDivision=%d\n",_curPos,rangeAdjuster, metronomePosition, MusEGlobal::config.midiDivision);
  setString(_curPos);
  update();
  metronome->update();
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void BigTime::resizeEvent(QResizeEvent *ev)
{
  QWidget::resizeEvent(ev);
  dwin->resize(ev->size());    
  QFont f    = dwin->font();
  QFontMetrics fm(f);
  int fs     = f.pixelSize();
  int hspace = 20;
  int tw     = fm.width(QString("000:00:00:00"));
  
  fs         = ((ev->size().width() - hspace*2)*fs) / tw;
 
  // set min/max
  if (fs < 10)
        fs = 10;
  else if (fs > 256)
        fs = 256;
        
  QString fstr = QString("font-size:%1px; font-family:'Courier'; ").arg(fs);  // Tim p4.0.8
  dwin->setStyleSheet(fstr);
  setBgColor(MusEGlobal::config.bigTimeBackgroundColor);
  setFgColor(MusEGlobal::config.bigTimeForegroundColor);
  
  int digitWidth = dwin->fontMetrics().width(QString("0"));
  int vspace = (ev->size().height() - (fs*2)) / 3;
  int tickY = vspace;
  
  int timeY = vspace*2 + fs;
  int absTickY = tickY;
  int absFrameY = timeY;
  barLabel->resize(digitWidth*4, fs);
  beatLabel->resize(digitWidth*2, fs);
  tickLabel->resize(digitWidth*3, fs);
  minLabel->resize(digitWidth*3, fs);
  secLabel->resize(digitWidth*2, fs);
  frameLabel->resize(digitWidth*2, fs);
  subFrameLabel->resize(digitWidth*2, fs);

  absTickLabel->resize(digitWidth*10, fs);
  absFrameLabel->resize(digitWidth*10, fs);
  sep1->resize(digitWidth, fs);
  sep2->resize(digitWidth, fs);
  sep3->resize(digitWidth, fs);
  sep4->resize(digitWidth, fs);
  sep5->resize(digitWidth, fs);

  barLabel->move(		hspace + (digitWidth*0), tickY);
  sep1->move(		hspace + (digitWidth*4), tickY);
  beatLabel->move(	hspace + (digitWidth*5), tickY);
  sep2->move(		hspace + (digitWidth*7), tickY);
  tickLabel->move(	hspace + (digitWidth*8), tickY);

  minLabel->move(   hspace + (digitWidth*0), timeY);
  sep3->move(   hspace + (digitWidth*3), timeY);
  secLabel->move(   hspace + (digitWidth*4), timeY);
  sep4->move(   hspace + (digitWidth*6), timeY);
  frameLabel->move( hspace + (digitWidth*7), timeY);
  sep5->move(   hspace + (digitWidth*9), timeY);
  subFrameLabel->move( hspace + (digitWidth*10), timeY);
  
  absTickLabel->move( hspace + (digitWidth*0), absTickY);
  absFrameLabel->move( hspace + (digitWidth*0), absFrameY);

  metronome->move(0,dwin->height() - dwin->height()/10);
  metronome->resize(dwin->width(),dwin->height()/10);
}

//---------------------------------------------------------
//   setForegroundColor
//---------------------------------------------------------

void BigTime::setFgColor(QColor c)
{
    QPalette newpalette(palette());
    newpalette.setColor(QPalette::Foreground, c);
    setPalette(newpalette);

    barLabel->setPalette(newpalette);
    beatLabel->setPalette(newpalette);
    tickLabel->setPalette(newpalette);
    //hourLabel->setPalette(newpalette);
    minLabel->setPalette(newpalette);
    secLabel->setPalette(newpalette);
    frameLabel->setPalette(newpalette);
    subFrameLabel->setPalette(newpalette);

    absTickLabel->setPalette(newpalette);
    absFrameLabel->setPalette(newpalette);
    sep1->setPalette(newpalette);
    sep2->setPalette(newpalette);
    sep3->setPalette(newpalette);
    sep4->setPalette(newpalette);
    sep5->setPalette(newpalette);
}

//---------------------------------------------------------
//   setBackgroundColor
//---------------------------------------------------------

void BigTime::setBgColor(QColor c)
{
    QPalette newpalette(palette());
    newpalette.setColor(QPalette::Window, c);
    setPalette(newpalette);

    barLabel->setPalette(newpalette);
    beatLabel->setPalette(newpalette);
    tickLabel->setPalette(newpalette);
    //hourLabel->setPalette(newpalette);
    minLabel->setPalette(newpalette);
    secLabel->setPalette(newpalette);
    frameLabel->setPalette(newpalette);
    subFrameLabel->setPalette(newpalette);

    absTickLabel->setPalette(newpalette);
    absFrameLabel->setPalette(newpalette);
    sep1->setPalette(newpalette);
    sep2->setPalette(newpalette);
    sep3->setPalette(newpalette);
    sep4->setPalette(newpalette);
    sep5->setPalette(newpalette);

    setPalette(newpalette);
}


} // namespace MusEGui
