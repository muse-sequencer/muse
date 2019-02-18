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

#include <climits>

#include "sync.h"
#include "posedit.h"
#include "sig.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyle>
#include <QString>
#include <QStyleOption>
#include <QMenu>

namespace MusEGui {

      using MusEGlobal::sigmap;

//---------------------------------------------------------
//   PosEdit
//---------------------------------------------------------

PosEdit::PosEdit(QWidget* parent, const MusECore::TimeFormatOptionsStruct& options)
   : QAbstractSpinBox(parent)
      {
      //_formatted = options & MusECore::TimeFormatFormatted;
      //_smpte = options & MusECore::TimeFormatFrames;
      _returnMode = false; 
      cur_minute = cur_sec = cur_frame = cur_subframe = 0;
      cur_bar = cur_beat = cur_tick = 0;
      
      validator = new QIntValidator(this);
      
      initialized = false;
      setReadOnly(false);
      
      setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      
// REMOVE Tim. clip. Changed.
//       setSmpte(false);
      setTimeFormatOptions(options);
      
//       lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
//       connect(lineEdit(), SIGNAL(customContextMenuRequested(QPoint)), 
//             SLOT(customMenuRequested(QPoint)));      
      }

QSize PosEdit::sizeHint() const
	{
    if(const QStyle* st = style())
    {
      st = st->proxy();
      
      QStyleOptionSpinBox option;
      option.initFrom(this);
      option.rect = rect();
      option.state = QStyle::State_Active | QStyle::State_Enabled;
      const QRect b_rect = st->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp);
      
      QFontMetrics fm = fontMetrics();
      const int fw = st->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
      int h  = fm.height() + fw * 2;
      int w = fw * 2 + b_rect.width();
      if(formatted())
      {
//         if (smpte())
        if (framesDisplay())
              w  += fm.width(QString("000:00:00:00"));
        else
              w  += fm.width(QString("0000.00.000"));
      }
      else
      {
//         w  += fm.width(QString("000000000000"));
        w  += fm.width(QString("0000000000A")); // Ten digits plus unit indicator.
      }
      return QSize(w, h).expandedTo(QApplication::globalStrut());
    }
    return QSize(20, 20).expandedTo(QApplication::globalStrut());      
	}

//---------------------------------------------------------
//   event
//    filter Tab and Backtab key events
//---------------------------------------------------------

bool PosEdit::event(QEvent* event)
{
      if (event->type() == QEvent::KeyPress) 
      {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Return) 
            {
              //printf("key press event Return\n");   
              //enterPressed();
              bool changed = finishEdit();
              if(changed || _returnMode)   // Force valueChanged if return mode set, even if not modified.
              {
                emit valueChanged(_pos);
              }
              emit returnPressed();
              emit editingFinished();
              return true;
            }
            
            if (ke->key() == Qt::Key_Escape) 
            {
              //printf("key press event Escape\n");   
              if(lineEdit())
                lineEdit()->undo(); 
              // "By default, isAccepted() is set to true, but don't rely on this as subclasses may 
              //   choose to clear it in their constructor."
              // Just to be sure. Otherwise escape will close a midi editor for example, which is annoying.
              ke->setAccepted(true);
              emit escapePressed();
              return true;
            }
            
            int segment = curSegment();
            if (ke->key() == Qt::Key_Backtab) 
            {
              if(formatted())
              {
//                   if (smpte()) {
                  if (framesDisplay()) {
                        if (segment == 3) {
                              lineEdit()->setSelection(7, 2);
                              return true;
                              }
                        else if (segment == 2) {
                              lineEdit()->setSelection(4, 2);
                              return true;
                              }
                        else if (segment == 1) {
                              lineEdit()->setSelection(0, 3);
                              return true;
                              }
                        }
                  else {
                        if (segment == 2) {
                              lineEdit()->setSelection(5, 2);
                              return true;
                              }
                        if (segment == 1) {
                              lineEdit()->setSelection(0, 4);
                              return true;
                              }
                        }
              }
              else
              {
                int cur_pos = lineEdit()->cursorPosition();
                if(cur_pos > 0)
                  --cur_pos;
                lineEdit()->setSelection(cur_pos, 1);
                return true;
              }
            }
            if (ke->key() == Qt::Key_Tab) 
            {
              if(formatted())
              {
//                   if (smpte()) {
                  if (framesDisplay()) {
                        if (segment == 0) {
                              lineEdit()->setSelection(4, 2);
                              return true;
                              }
                        else if (segment == 1) {
                              lineEdit()->setSelection(7, 2);
                              return true;
                              }
                        else if (segment == 2) {
                              lineEdit()->setSelection(10, 2);
                              return true;
                              }
                        }
                  else {
                        if (segment == 0) {
                              lineEdit()->setSelection(5, 2);
                              return true;
                              }
                        if (segment == 1) {
                              lineEdit()->setSelection(8, 3);
                              return true;
                              }
                        }
              }
              else
              {
                // Normal tab. Let parent handle it.
                event->ignore();
              }
            }
      }
      else if (event->type() == QEvent::FocusIn) 
      {
          // REMOVE Tim. clip. Added. Diagnostics.
          fprintf(stderr, "FocusIn\n");
          QFocusEvent* fe = static_cast<QFocusEvent*>(event);
          QAbstractSpinBox::focusInEvent(fe);
          int segment = curSegment();
          if(formatted())
          {
            // FIXME: frames vs. ticks?
            switch(segment) {
                  case 0:  lineEdit()->setSelection(0,4); break;
                  case 1:  lineEdit()->setSelection(5,2); break;
                  case 2:  lineEdit()->setSelection(8,3); break;
                  }
          }
          else
          {
            lineEdit()->selectAll();
          }
          return true;
      }
      else if (event->type() == QEvent::FocusOut) 
      {
          // REMOVE Tim. clip. Added. Diagnostics.
          fprintf(stderr, "FocusOut\n");
          QFocusEvent* fe = static_cast<QFocusEvent*>(event);
          QAbstractSpinBox::focusOutEvent(fe);
          if(finishEdit())
            emit valueChanged(_pos);
          emit lostFocus();        
          emit editingFinished();  
          return true;
      }
      
      return QAbstractSpinBox::event(event);
}

//---------------------------------------------------------
//   updateInputMask
//---------------------------------------------------------

void PosEdit::updateInputMask()
{
  if(formatted())
  {
//     if (smpte())
    if (framesDisplay())
          lineEdit()->setInputMask("999:99:99:99;0");
    else
          lineEdit()->setInputMask("9999.99.999;0");
  }
  else
  {
          //lineEdit()->setInputMask("999999999999;0");
//           lineEdit()->setInputMask("000000000009");
    // Ten digits plus unit indicator.
    lineEdit()->setInputMask("0000000009X");
  }
}

//---------------------------------------------------------
//   toggleTimeFormatOptions
//---------------------------------------------------------

void PosEdit::toggleTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options, bool set)
{
  _timeFormatOptions.setOptions(options, set);
  _pos.setType(smpte() ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
  updateInputMask();
  updateValue();
}

//---------------------------------------------------------
//   setTimeFormatOptions
//---------------------------------------------------------

void PosEdit::setTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options)
{
  _timeFormatOptions = options;
  _pos.setType(smpte() ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
  updateInputMask();
  updateValue();
}

//---------------------------------------------------------
//   setSmpte
//---------------------------------------------------------

void PosEdit::setSmpte(bool f)
      {
      toggleTimeFormatOptions(MusECore::TimeModeFrames, f);
      updateInputMask();
      updateValue();
      }

//---------------------------------------------------------
//   setFormatted
//---------------------------------------------------------

void PosEdit::setFormatted(bool f)
{
  toggleTimeFormatOptions(MusECore::TimeFormatFormatted, f);
  updateInputMask();
  updateValue();
}

void PosEdit::setFramesDisplay(bool f)
{
  toggleTimeFormatOptions(MusECore::TimeDisplayFrames, f);
  updateInputMask();
  updateValue();
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosEdit::setValue(const MusECore::Pos& time)
{
      if(formatted() && _pos == time)
      {
        // Must check whether actual values dependent on tempo or sig changed...
//         if (smpte()) {
        if (framesDisplay()) {
              int minute, sec, frame, subframe;
              time.msf(&minute, &sec, &frame, &subframe);
              if(minute != cur_minute || sec != cur_sec || frame != cur_frame || subframe != cur_subframe)
                updateValue();
              }
        else {
              int bar, beat, tick;
              time.mbt(&bar, &beat, &tick);
              if(bar != cur_bar || beat != cur_beat || tick != cur_tick)
                updateValue();
              }
      }      
      else
      {
        // We want to keep the user's desired type.
        // Use this instead of assignment which sets everything.
        //_pos.setPos(time);
        _pos.setTickAndFrame(time);
        updateValue();
      }
}

void PosEdit::setValue(const QString& s)
      {
      MusECore::Pos time(s);
      setValue(time);
      }

void PosEdit::setValue(int t)
      {
      MusECore::Pos time(t);
      setValue(time);
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void PosEdit::updateValue()
      {
      QString s;
      const MusECore::Pos p(_pos);
      const bool fr_disp = framesDisplay();

      if(formatted())
      {
//         if (smpte()) {
        if (fr_disp) {
              p.msf(&cur_minute, &cur_sec, &cur_frame, &cur_subframe);
              s = QString("%1:%2:%3:%4")
                  .arg(cur_minute,   3, 10, QLatin1Char('0'))
                  .arg(cur_sec,      2, 10, QLatin1Char('0'))
                  .arg(cur_frame,    2, 10, QLatin1Char('0'))
                  .arg(cur_subframe, 2, 10, QLatin1Char('0'));
              }
        else {
              p.mbt(&cur_bar, &cur_beat, &cur_tick);
              s = QString("%1.%2.%3")
                  .arg(cur_bar + 1,  4, 10, QLatin1Char('0'))
                  .arg(cur_beat + 1, 2, 10, QLatin1Char('0'))
                  .arg(cur_tick,     3, 10, QLatin1Char('0'));
              }
      }
      else
      {
        //s = QString("%1").arg(_pos.posValue(), 12, 10, QLatin1Char('0'));
        //s = QString("%1").arg(_pos.posValue(), 0, 10);
        const MusECore::Pos::TType ttype = fr_disp ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS;
//         s = QString("%1").arg(p.posValue(ttype), 12, 10);
        s = QString("%1%2").arg(p.posValue(ttype), 10, 10).arg(fr_disp ? '.' : ':');
      }

//       lineEdit()->clear();
//       lineEdit()->end(false);
//       lineEdit()->setModified(false);
      lineEdit()->setText(s);
      }

//---------------------------------------------------------
//   stepEnables
//---------------------------------------------------------

QAbstractSpinBox::StepEnabled PosEdit::stepEnabled() const
      {
//       const bool frame_mode = smpte() && framesDisplay();
      int segment = curSegment();
      QAbstractSpinBox::StepEnabled en = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;

      if(formatted())
      {
//         if (frame_mode) {
        if (framesDisplay()) {
              int minute, sec, frame, subframe;
              _pos.msf(&minute, &sec, &frame, &subframe);
              switch(segment) {
                    case 0:
                          if (minute == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          break;
                    case 1:
                          if (sec == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          else if (sec == 59)
                                en &= ~QAbstractSpinBox::StepUpEnabled;
                          break;
                    case 2:
                          if (frame == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          else
                          {
                            int nf = 23;    // 24 frames sec
                            switch(MusEGlobal::mtcType) {
                                  //case 0:     // 24 frames sec
                                  //      nf = 23;
                                  //      break;
                                  case 1:
                                        nf = 24;  // 25 frames sec
                                        break;
                                  case 2:     // 30 drop frame
                                  case 3:     // 30 non drop frame
                                        nf = 29;
                                        break;
                                  default:
                                        break;      
                                  }
                            //if (frame == 23)
                            if (frame >= nf)
                                en &= ~QAbstractSpinBox::StepUpEnabled;
                          }      
                          break;
                    case 3:
                          if (subframe == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          else if (subframe == 99)
                                en &= ~QAbstractSpinBox::StepUpEnabled;
                          break;
                    }
              }
        else {
              int bar, beat;
              unsigned tick;
              MusEGlobal::sigmap.tickValues(_pos.tick(), &bar, &beat, &tick);
              unsigned tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
              unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
              int bm = tm / tb;

              switch (segment) {
                    case 0:
                          if (bar == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          break;
                    case 1:
                          if (beat == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          else {
                                if (beat >= (bm-1))
                                      en &= ~QAbstractSpinBox::StepUpEnabled;
                                }
                          break;
                    case 2:
                          if (tick == 0)
                                en &= ~QAbstractSpinBox::StepDownEnabled;
                          else {
                                if (tick >= (tb-1))
                                      en &= ~QAbstractSpinBox::StepUpEnabled;
                                }
                          break;
                    }
              }
      }
      else
      {
        en = QAbstractSpinBox::StepNone;
        if(_pos.posValue() > 0)
          en = QAbstractSpinBox::StepDownEnabled;
        if(_pos.posValue() < UINT_MAX)
          en |= QAbstractSpinBox::StepUpEnabled;
      }
      return en;
      }

//---------------------------------------------------------
//   fixup
//---------------------------------------------------------

void PosEdit::fixup(QString& /*input*/) const
      {
      //printf("fixup <%s>\n", input.toLatin1().constData()); 
      }

//---------------------------------------------------------
//   validate
//---------------------------------------------------------

QValidator::State PosEdit::validate(QString& s,int& /*i*/) const
{
      //printf("validate string:%s int:%d\n", s.toLatin1().data(), i);  
      //printf("validate string:%s\n", s.toLatin1().data());  

      QValidator::State state;
      QValidator::State rv = QValidator::Acceptable;
      // "By default, the pos parameter is not used by this [QIntValidator] validator."
      int dpos = 0;    
      
      if(formatted())
      {
//         QStringList sl = s.split(smpte() ? ':' : '.');
        QStringList sl = s.split(framesDisplay() ? ':' : '.');
//         if (smpte()) 
        if (framesDisplay()) 
        {
          if(sl.size() != 4)
          {
            printf("validate smpte string:%s sections:%d != 4\n", s.toLatin1().data(), sl.size());  
            return QValidator::Invalid;
          }  
          
          validator->setRange(0, 999);
          state = validator->validate(sl[0], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(0, 59);
          state = validator->validate(sl[1], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          int nf = 23;      // 24 frames sec
          switch(MusEGlobal::mtcType) {
                //case 0:     // 24 frames sec
                //      nf = 23;
                //      break;
                case 1:
                      nf = 24;  // 25 frames sec
                      break;
                case 2:     // 30 drop frame
                case 3:     // 30 non drop frame
                      nf = 29;
                      break;
                default:
                      break;      
                }
          validator->setRange(0, nf);
          state = validator->validate(sl[2], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(0, 99);
          state = validator->validate(sl[3], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
        }
        else
        {
          if(sl.size() != 3)
          {
            printf("validate bbt string:%s sections:%d != 3\n", s.toLatin1().data(), sl.size());  
            return QValidator::Invalid;
          }
            
          int tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
          unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
          if (tm==0)
            return QValidator::Invalid;
          int bm = tm / tb;

          validator->setRange(1, 9999);
          //printf("validate substring 0:%s\n", sl[0].toLatin1().data());  
          // Special hack because validator says 0000 is intermediate.
          if(sl[0] == "0000")
            return QValidator::Invalid;
          state = validator->validate(sl[0], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(1, bm);
          //printf("validate substring 1:%s\n", sl[1].toLatin1().data());  
          // Special hack because validator says 00 is intermediate.
          if(sl[1] == "00")
            return QValidator::Invalid;
          state = validator->validate(sl[1], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(0, tb-1);
          //printf("validate substring 2:%s\n", sl[2].toLatin1().data());  
          state = validator->validate(sl[2], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
        }
      }
      else
      {
        //// Normal input. Let the base do it.
        //return QAbstractSpinBox::validate(s, i);
        
        // FIXME  We need unsigned int range! This limits the available range by half.
        //validator->setRange(0, INT_MAX);
        //state = validator->validate(s, dpos);
        rv = QValidator::Acceptable;
      }
      return rv;
}

//---------------------------------------------------------
//   curSegment
//---------------------------------------------------------

int PosEdit::curSegment() const
      {
      QLineEdit* le = lineEdit();
      int pos = le->cursorPosition();
      int segment = -1;

      if(formatted())
      {
//         if (smpte()) {
        if (framesDisplay()) {
              if (pos >= 0 && pos <= 3)
                    segment = 0;
              else if (pos >= 4 && pos <= 6)
                    segment = 1;
              else if (pos >= 7 && pos <= 9)
                    segment = 2;
              else if (pos >= 10)
                    segment = 3;
              }
        else {
              if (pos >= 0 && pos <= 4)
                    segment = 0;
              else if (pos >= 5 && pos <= 7)
                    segment = 1;
              else if (pos >= 8)
                    segment = 2;
              else
                    printf("curSegment = -1, pos %d\n", pos);
              }
      }
      else
      {
        if (pos >= 0 && pos <= 9)
          segment = 0;
        else
          segment = 1;
      }
      return segment;
      }

//---------------------------------------------------------
//   stepBy
//---------------------------------------------------------

void PosEdit::stepBy(int steps)
      {
      const bool frame_mode = smpte() && framesDisplay();
      const MusECore::Pos p(_pos);
      int segment = 0;
      int selPos = 0;
      int selLen = 0;
      if(formatted())
      //if(formatted() && ((smpte() && framesDisplay()) || (!smpte() && !framesDisplay())))
      //if(formatted() && (smpte() || !framesDisplay()))
      {
        segment = curSegment();
        bool changed = false;

//         if (smpte()) {
        if (framesDisplay()) {
//         if (frame_mode) {
          
            //if(smpte())
            //{
              int minute, sec, frame, subframe;
              p.msf(&minute, &sec, &frame, &subframe);
              switch(segment) {
                    case 0:
                          minute += steps;
                          if (minute < 0)
                                minute = 0;
                          break;
                    case 1:
                          sec += steps;
                          if (sec < 0)
                                sec = 0;
                          if (sec > 59)
                                sec = 59;
                          break;
                    case 2:
                          {
                            int nf = 23;      // 24 frames sec
                            switch(MusEGlobal::mtcType) {
                                  //case 0:     // 24 frames sec
                                  //      nf = 23;
                                  //      break;
                                  case 1:
                                        nf = 24;    // 25 frames sec
                                        break;
                                  case 2:     // 30 drop frame
                                  case 3:     // 30 non drop frame
                                        nf = 29;
                                        break;
                                  default:
                                        break;      
                                  }
                            frame += steps;
                            if (frame < 0)
                                  frame = 0;
                            //if (frame > 24)         //TD frame type?
                            //      frame = 24;
                            if (frame > nf)         
                                  frame = nf;
                          }
                          break;
                    case 3:
                          subframe += steps;
                          if (subframe < 0)
                                subframe = 0;
                          if (subframe > 99)
                                subframe = 99;
                          break;
                    default:
                          return;
                    }
              MusECore::Pos newPos(minute, sec, frame, subframe);
              //unsigned int new_f = newPos.frame();
              if(!smpte())
              {
                newPos.setType(MusECore::Pos::TICKS);
                // Invalidate to get a new frame.
//                 //newPos.invalidSn();
//                 newPos.setType(MusECore::Pos::FRAMES);
                // Make sure there's at least enough for one tick increment.
                if(steps > 0 && newPos == _pos)
                  newPos += 1;
              }
                //new_f = MusECore::Pos(new_f, false);
              if (!(newPos == _pos)) {
                    changed = true;
                    //_pos = newPos;
                    _pos.setPos(newPos);
                    }
              }
        else {
              int bar, beat, tick;
              p.mbt(&bar, &beat, &tick);

              int tb = MusEGlobal::sigmap.ticksBeat(p.tick());
              //int tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
              unsigned tm = MusEGlobal::sigmap.ticksMeasure(p.tick());
              //unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
              int bm = tm / tb;

              switch(segment) {
                    case 0:
                          bar += steps;
                          if (bar < 0)
                                bar = 0;
                          break;
                    case 1:
                          beat += steps;
                          if (beat < 0)
                                beat = 0;
                          else if (beat >= bm)
                                beat = bm - 1;
                          break;
                    case 2:
                          tick += steps;
                          if (tick < 0)
                                tick = 0;
                          else if (tick >= tb)
                                tick = tb -1;
                          break;
                    default:
                          return;
                    }
              MusECore::Pos newPos(bar, beat, tick);
              if (!(newPos == _pos)) {
                    changed = true;
                    //_pos = newPos;
                    _pos.setPos(newPos);
                    }
              }
              
        if (changed) {
              updateValue();
              emit valueChanged(_pos);
              }
        //}
      }
      else
      {
        const unsigned int cur_val = frame_mode ? p.frame() : p.tick();
        if(steps >= 0)
        {
          const unsigned int avail = UINT_MAX - cur_val;
          if((unsigned int)steps > avail)
            steps = avail;
        }
        else
        {
          if((unsigned int)-steps > cur_val)
            steps = -cur_val;
        }
        const MusECore::Pos new_p(cur_val + steps, !frame_mode);
        _pos.setPos(new_p);
        updateValue();
        emit valueChanged(_pos);
      }

      
      
      
      
      
      if(formatted())
      {
        if(framesDisplay())
        {
          switch(segment) {
                case 0:
                      selPos = 0;
                      selLen = 3;
                      break;
                case 1:
                      selPos = 4;
                      selLen = 2;
                      break;
                case 2:
                      {
                        selPos = 7;
                        selLen = 2;
                      }
                      break;
                case 3:
                      selPos = 10;
                      selLen = 2;
                      break;
                }
        } 
        else
        {
          switch(segment) {
                case 0:
                      selPos = 0;
                      selLen = 4;
                      break;
                case 1:
                      selPos = 5;
                      selLen = 2;
                      break;
                case 2:
                      selPos = 8;
                      selLen = 3;
                      break;
                }
        }
        lineEdit()->setSelection(selPos, selLen);
      }
      else
      {
//         lineEdit()->selectAll();
        lineEdit()->setSelection(0, 10);
      }
      
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PosEdit::paintEvent(QPaintEvent* event) 
{
  if (!initialized)
        updateValue();
  initialized = true;
  QAbstractSpinBox::paintEvent(event);
}

//---------------------------------------------------------
//   finishEdit
//   Return true if position changed.
//---------------------------------------------------------

bool PosEdit::finishEdit()
{
      bool ok;
      bool changed = false;
      if(formatted())
      {
        // If our validator did its job correctly, the entire line edit text should be valid now...
        
//         QStringList sl = text().split(smpte() ? ':' : '.');
        QStringList sl = text().split(framesDisplay() ? ':' : '.');
//         if (smpte()) 
        if (framesDisplay()) 
        {
          if(sl.size() != 4)
          {
            printf("finishEdit smpte string:%s sections:%d != 4\n", text().toLatin1().data(), sl.size());  
            return false;
          }  
          
          MusECore::Pos newPos(sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt());
          if (!(newPos == _pos)) 
          {
            changed = true;
            //_pos = newPos;
            _pos.setPos(newPos);
          }
        }
        else
        {
          if(sl.size() != 3)
          {
            printf("finishEdit bbt string:%s sections:%d != 3\n", text().toLatin1().data(), sl.size());  
            return false;
          }
            
          MusECore::Pos newPos(sl[0].toInt() - 1, sl[1].toInt() - 1, sl[2].toInt());
          if (!(newPos == _pos)) 
          {
            changed = true;
            //_pos = newPos;
            _pos.setPos(newPos);
          }
        }
      }
      else
      {
        // Make sure we leave enough for all the digits!
        // 32 is not enough for this display. Also, plan for the future of 64 bit frame/tick etc.
        const QString txt = text();
        // Chopped undefined behaviour if string not long enough.
        if(!txt.isEmpty())
        {
          // Discard (what should be) the unit character.
          const unsigned long long new_val = txt.chopped(1).toULongLong(&ok);
//           if(ok && new_val != _pos.posValue())
          if(ok)
          {
            MusECore::Pos new_p(new_val, !framesDisplay());
            if(!smpte())
              new_p.setType(MusECore::Pos::TICKS);
            if(new_p != _pos)
            {
              changed = true;
  //             _pos.setPosValue(new_val);
              _pos.setPos(new_p);
            }
          }
        }
      }
  return changed;
}

// void PosEdit::customMenuRequested(QPoint pos)
// {
//   fprintf(stderr, "PosEdit::customMenuRequested\n");
//   QMenu* menu = lineEdit()->createStandardContextMenu();
//   menu->addAction(tr("My menu item"));
//   menu->exec(mapToGlobal(pos));
//   delete menu;
// }

void PosEdit::contextMenuEvent(QContextMenuEvent* event)
{
  fprintf(stderr, "PosEdit::contextMenuEvent\n");
  QMenu* menu = lineEdit()->createStandardContextMenu();
  if(!menu)
    return;

  event->accept();

  const QAbstractSpinBox::StepEnabled sen = stepEnabled();
  
  QAction* act = nullptr;
  
  if(_timeFormatOptions.flagsSet((MusECore::TimeFormatUserFormat | MusECore::TimeFormatUserFormat)))
    menu->addSeparator();
  
  if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserMode))
  {
    act = menu->addAction(tr("Mode: Frames vs. ticks"));
    act->setData(ContextIdMode);
    act->setCheckable(true);
    act->setChecked(smpte());
  }

  if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserDisplayMode))
  {
    act = menu->addAction(tr("Display: Frames vs. ticks"));
    act->setData(ContextIdDisplayMode);
    act->setCheckable(true);
    act->setChecked(framesDisplay());
  }

  if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserFormat))
  {
    act = menu->addAction(tr("Format (as BBT or MSF)"));
    act->setData(ContextIdFormat);
    act->setCheckable(true);
    act->setChecked(formatted());
  }
  
  menu->addSeparator();

  act = menu->addAction(tr("Step up"));
  act->setData(ContextIdStepUp);
  act->setEnabled(sen & QAbstractSpinBox::StepUpEnabled);

  act = menu->addAction(tr("Step down"));
  act->setData(ContextIdStepDown);
  act->setEnabled(sen & QAbstractSpinBox::StepDownEnabled);
  
  act = menu->exec(event->globalPos());

  int idx = -1;
  bool is_checked = false;
  if(act && act->data().isValid())
  {
    idx = act->data().toInt();
    is_checked = act->isChecked();
  }
  delete menu;

  if(idx == -1)
    return;

  switch(idx)
  {
    case ContextIdStepUp:
      stepUp();
    break;

    case ContextIdStepDown:
      stepDown();
    break;

    case ContextIdMode:
      toggleTimeFormatOptions(MusECore::TimeModeFrames, is_checked);
    break;

    case ContextIdDisplayMode:
      toggleTimeFormatOptions(MusECore::TimeDisplayFrames, is_checked);
    break;

    case ContextIdFormat:
      toggleTimeFormatOptions(MusECore::TimeFormatFormatted, is_checked);
    break;
  }
}

} // namespace MusEGui


