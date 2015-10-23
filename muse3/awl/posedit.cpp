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

#include "al/al.h"
#include "awl.h"
#include "posedit.h"
#include "al/sig.h"
//#include "sig.h"
//#include "sync.h"  // Tim.

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyle>
#include <QString>

namespace MusEGlobal {
extern int mtcType;
}

namespace Awl {

      ///using AL::mtcType;
      using AL::sigmap;

//---------------------------------------------------------
//   PosEdit
//---------------------------------------------------------

PosEdit::PosEdit(QWidget* parent)
   : QAbstractSpinBox(parent)
      {
      _returnMode = false; 
      cur_minute = cur_sec = cur_frame = cur_subframe = 0;
      cur_bar = cur_beat = cur_tick = 0;
      
      validator = new QIntValidator(this);
      
      initialized = false;
      setReadOnly(false);
      setSmpte(false);
      
      //connect(this, SIGNAL(editingFinished()), SLOT(finishEdit()));
      //connect(this, SIGNAL(returnPressed()), SLOT(enterPressed()));
      }

QSize PosEdit::sizeHint() const
	{
      //QFontMetrics fm(font());
      QFontMetrics fm = fontMetrics();
      int fw = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
      int h  = fm.height() + fw * 2;
      int w = fw * 4 + 10;	// HACK: 10 = spinbox up/down arrows
      if (_smpte)
            w  += 2 + fm.width('9') * 9 + fm.width(':') * 3 + fw * 4;
      else
            w  += 2 + fm.width('9') * 9 + fm.width('.') * 2 + fw * 4;
      return QSize(w, h).expandedTo(QApplication::globalStrut());
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
                  if (_smpte) {
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
            if (ke->key() == Qt::Key_Tab) 
            {
                  if (_smpte) {
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
      }
      else if (event->type() == QEvent::FocusIn) 
      {
          QFocusEvent* fe = static_cast<QFocusEvent*>(event);
          QAbstractSpinBox::focusInEvent(fe);
          int segment = curSegment();
          switch(segment) {
                case 0:  lineEdit()->setSelection(0,4); break;
                case 1:  lineEdit()->setSelection(5,2); break;
                case 2:  lineEdit()->setSelection(8,3); break;
                }
          return true;
      }
      else if (event->type() == QEvent::FocusOut) 
      {
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
//   setSmpte
//---------------------------------------------------------

void PosEdit::setSmpte(bool f)
      {
      _smpte = f;
      if (_smpte)
            //lineEdit()->setInputMask("999:99:99:99");
            lineEdit()->setInputMask("999:99:99:99;0");
      else
            //lineEdit()->setInputMask("9999.99.999");
            lineEdit()->setInputMask("9999.99.999;0");
      updateValue();
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosEdit::setValue(const MusECore::Pos& time)
{
      if(_pos == time)
      {
        // Must check whether actual values dependent on tempo or sig changed...
        if (_smpte) {
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
        _pos = time;
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
      char buffer[64];
      if (_smpte) {
            _pos.msf(&cur_minute, &cur_sec, &cur_frame, &cur_subframe);
            sprintf(buffer, "%03d:%02d:%02d:%02d", cur_minute, cur_sec, cur_frame, cur_subframe);
            }
      else {
            _pos.mbt(&cur_bar, &cur_beat, &cur_tick);
            sprintf(buffer, "%04d.%02d.%03d", cur_bar+1, cur_beat+1, cur_tick);
            }
      lineEdit()->setText(buffer);
      }

//---------------------------------------------------------
//   stepEnables
//---------------------------------------------------------

QAbstractSpinBox::StepEnabled PosEdit::stepEnabled() const
      {
      int segment = curSegment();
      QAbstractSpinBox::StepEnabled en = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;

      if (_smpte) {
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
            AL::sigmap.tickValues(_pos.tick(), &bar, &beat, &tick);
            //sigmap.tickValues(_pos.tick(), &bar, &beat, &tick);
            unsigned tb = AL::sigmap.ticksBeat(_pos.tick());
            //unsigned tb = sigmap.ticksBeat(_pos.tick());
            unsigned tm = AL::sigmap.ticksMeasure(_pos.tick());
            //unsigned tm = sigmap.ticksMeasure(_pos.tick());
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
      
      QStringList sl = s.split(_smpte ? ':' : '.');
      QValidator::State state;
      QValidator::State rv = QValidator::Acceptable;
      // "By default, the pos parameter is not used by this [QIntValidator] validator."
      int dpos = 0;    
      
      if (_smpte) 
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
          
        int tb = AL::sigmap.ticksBeat(_pos.tick());
        unsigned tm = AL::sigmap.ticksMeasure(_pos.tick());
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

      if (_smpte) {
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
      return segment;
      }

//---------------------------------------------------------
//   stepBy
//---------------------------------------------------------

void PosEdit::stepBy(int steps)
      {
      int segment = curSegment();
      int selPos;
      int selLen;

      bool changed = false;

      if (_smpte) {
             int minute, sec, frame, subframe;
            _pos.msf(&minute, &sec, &frame, &subframe);
            switch(segment) {
                  case 0:
                        minute += steps;
                        if (minute < 0)
                              minute = 0;
                        selPos = 0;
                        selLen = 3;
                        break;
                  case 1:
                        sec += steps;
                        if (sec < 0)
                              sec = 0;
                        if (sec > 59)
                              sec = 59;
                        selPos = 4;
                        selLen = 2;
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
                          selPos = 7;
                          selLen = 2;
                        }
                        break;
                  case 3:
                        subframe += steps;
                        if (subframe < 0)
                              subframe = 0;
                        if (subframe > 99)
                              subframe = 99;
                        selPos = 10;
                        selLen = 2;
                        break;
                  default:
                        return;
                  }
            MusECore::Pos newPos(minute, sec, frame, subframe);
            if (!(newPos == _pos)) {
                  changed = true;
                  _pos = newPos;
                  }
            }
      else {
            int bar, beat, tick;
            _pos.mbt(&bar, &beat, &tick);

            int tb = AL::sigmap.ticksBeat(_pos.tick());
            //int tb = sigmap.ticksBeat(_pos.tick());
            unsigned tm = AL::sigmap.ticksMeasure(_pos.tick());
            //unsigned tm = sigmap.ticksMeasure(_pos.tick());
            int bm = tm / tb;

            switch(segment) {
                  case 0:
                        bar += steps;
                        if (bar < 0)
                              bar = 0;
                        selPos = 0;
                        selLen = 4;
                        break;
                  case 1:
                        beat += steps;
                        if (beat < 0)
                              beat = 0;
                        else if (beat >= bm)
                              beat = bm - 1;
                        selPos = 5;
                        selLen = 2;
                        break;
                  case 2:
                        tick += steps;
                        if (tick < 0)
                              tick = 0;
                        else if (tick >= tb)
                              tick = tb -1;
                        selPos = 8;
                        selLen = 3;
                        break;
                  default:
                        return;
                  }
            MusECore::Pos newPos(bar, beat, tick);
            if (!(newPos == _pos)) {
                  changed = true;
                  _pos = newPos;
                  }
            }
      if (changed) {
            updateValue();
            emit valueChanged(_pos);
            }
      lineEdit()->setSelection(selPos, selLen);
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
      // If our validator did its job correctly, the entire line edit text should be valid now...
      
      bool changed = false;
      QStringList sl = text().split(_smpte ? ':' : '.');
      if (_smpte) 
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
          _pos = newPos;
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
          _pos = newPos;
        }
      }
  
  return changed;
}


} // namespace Awl


