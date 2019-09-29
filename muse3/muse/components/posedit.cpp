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
#include "menutitleitem.h"

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

PosEdit::PosEdit(QWidget* parent,
  const MusECore::Pos::TType& type,
  bool fixed_type,
//   bool quantize_value,
//   const MusECore::TimeFormatOptionsStruct& options
  const MusECore::TimeFormatOptions& options,
  const MusECore::TimeFormatOptions_t& allowed_time_formats
)
   : QAbstractSpinBox(parent)
      {
// REMOVE Tim. clip. Added.
      setKeyboardTracking(false);

      //_formatted = options & MusECore::TimeFormatFormatted;
      //_smpte = options & MusECore::TimeFormatFrames;
      _returnMode = false; 
      cur_minute = cur_sec = cur_frame = cur_subframe = 0;
      cur_msmu_minute = cur_msmu_sec =cur_msmu_msec = cur_msmu_usec = 0;
      cur_bar = cur_beat = cur_tick = 0;
      
      validator = new QIntValidator(this);
      
      initialized = false;
      setReadOnly(false);
      
      setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      
// REMOVE Tim. clip. Changed.
//       setSmpte(false);
      _allowedTimeFormats = allowed_time_formats;
      _fixed_type = fixed_type;
//       _quantizeValue = quantize_value;
      setType(type);
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
//       if(formatted())
//       {
// //         if (smpte())
//         if (framesDisplay())
//               w  += fm.width(QString("000:00:00:00"));
//         else
//               w  += fm.width(QString("0000.00.000"));
//       }
//       else
//       {
// //         w  += fm.width(QString("000000000000"));
//         w  += fm.width(QString("0000000000A")); // Ten digits plus unit indicator.
//       }
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatFrames:
        case MusECore::TimeFormatTicks:
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
          w  += fm.horizontalAdvance(QString("0000000000A")); // Ten digits plus unit indicator.
#else
          w  += fm.width(QString("0000000000A")); // Ten digits plus unit indicator.
#endif
        break;

        case MusECore::TimeFormatBBT:
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
          w  += fm.horizontalAdvance(QString("0000.00.000"));
#else
          w  += fm.width(QString("0000.00.000"));
#endif
        break;

        case MusECore::TimeFormatMSFS:
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
          w  += fm.horizontalAdvance(QString("000:00:00:00"));
#else
          w  += fm.width(QString("000:00:00:00"));
#endif
        break;

        case MusECore::TimeFormatMSMU:
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
          w  += fm.horizontalAdvance(QString("000:00:000:000"));
#else
          w  += fm.width(QString("000:00:000:000"));
#endif
        break;

        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
          //return QSize(20, 20).expandedTo(QApplication::globalStrut());
          return QAbstractSpinBox::sizeHint();
        break;
      }

      const QSize sz = QSize(w, h).expandedTo(QApplication::globalStrut());
      // REMOVE Tim. clip. Added. Diagnostics.
      fprintf(stderr, "PosEdit::sizeHint(): w:%d h:%d\n", sz.width(), sz.height());
      return sz;
    }
//     return QSize(20, 20).expandedTo(QApplication::globalStrut());
    return QAbstractSpinBox::sizeHint();
	}

// QSize PosEdit::minimumSizeHint() const
// {
//   return sizeHint();
// }

	
	
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
              //fprintf(stderr, "key press event Return\n");   
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
              //fprintf(stderr, "key press event Escape\n");   
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
//               if(formatted())
//               {
// //                   if (smpte()) {
//                   if (framesDisplay()) {
//                         if (segment == 3) {
//                               lineEdit()->setSelection(7, 3);
//                               return true;
//                               }
//                         else if (segment == 2) {
//                               lineEdit()->setSelection(4, 3);
//                               return true;
//                               }
//                         else if (segment == 1) {
//                               lineEdit()->setSelection(0, 4);
//                               return true;
//                               }
//                         }
//                   else {
//                         if (segment == 2) {
//                               lineEdit()->setSelection(5, 3);
//                               return true;
//                               }
//                         if (segment == 1) {
//                               lineEdit()->setSelection(0, 5);
//                               return true;
//                               }
//                         }
//               }
//               else
//               {
//                 int cur_pos = lineEdit()->cursorPosition();
//                 if(cur_pos > 0)
//                   --cur_pos;
//                 lineEdit()->setSelection(cur_pos, 1);
//                 return true;
//               }
              
              
              
              
              switch(_timeFormatOptions)
              {
                case MusECore::TimeFormatFrames:
                case MusECore::TimeFormatTicks:
                {
                  int cur_pos = lineEdit()->cursorPosition();
                  if(cur_pos > 0)
                    --cur_pos;
                  lineEdit()->setSelection(cur_pos, 1);
                  return true;
                }
                break;

                case MusECore::TimeFormatBBT:
                  if (segment == 2) {
                        lineEdit()->setSelection(5, 3);
                        return true;
                        }
                  if (segment == 1) {
                        lineEdit()->setSelection(0, 5);
                        return true;
                        }
                break;

                case MusECore::TimeFormatMSFS:
                  if (segment == 3) {
                        lineEdit()->setSelection(7, 3);
                        return true;
                        }
                  else if (segment == 2) {
                        lineEdit()->setSelection(4, 3);
                        return true;
                        }
                  else if (segment == 1) {
                        lineEdit()->setSelection(0, 4);
                        return true;
                        }
                break;

                case MusECore::TimeFormatMSMU:
                  if (segment == 3) {
                        lineEdit()->setSelection(7, 4);
                        return true;
                        }
                  else if (segment == 2) {
                        lineEdit()->setSelection(4, 3);
                        return true;
                        }
                  else if (segment == 1) {
                        lineEdit()->setSelection(0, 4);
                        return true;
                        }
                break;

                case MusECore::TimeFormatNoOptions:
                case MusECore::TimeFormatAll:
                  return true;
                break;
              }
            }
            if (ke->key() == Qt::Key_Tab) 
            {
//               if(formatted())
//               {
// //                   if (smpte()) {
//                   if (framesDisplay()) {
//                         if (segment == 0) {
//                               lineEdit()->setSelection(4, 3);
//                               return true;
//                               }
//                         else if (segment == 1) {
//                               lineEdit()->setSelection(7, 3);
//                               return true;
//                               }
//                         else if (segment == 2) {
//                               lineEdit()->setSelection(10, 2);
//                               return true;
//                               }
//                         }
//                   else {
//                         if (segment == 0) {
//                               lineEdit()->setSelection(5, 3);
//                               return true;
//                               }
//                         if (segment == 1) {
//                               lineEdit()->setSelection(8, 3);
//                               return true;
//                               }
//                         }
//               }
//               else
//               {
//                 // Normal tab. Let parent handle it.
//                 event->ignore();
//               }
              
              
              
              switch(_timeFormatOptions)
              {
                case MusECore::TimeFormatFrames:
                case MusECore::TimeFormatTicks:
                  // Normal tab. Let parent handle it.
                  event->ignore();
                break;

                case MusECore::TimeFormatBBT:
                  if (segment == 0) {
                        lineEdit()->setSelection(5, 3);
                        return true;
                        }
                  if (segment == 1) {
                        lineEdit()->setSelection(8, 3);
                        return true;
                        }
                break;

                case MusECore::TimeFormatMSFS:
                  if (segment == 0) {
                        lineEdit()->setSelection(4, 3);
                        return true;
                        }
                  else if (segment == 1) {
                        lineEdit()->setSelection(7, 3);
                        return true;
                        }
                  else if (segment == 2) {
                        lineEdit()->setSelection(10, 2);
                        return true;
                        }
                break;

                case MusECore::TimeFormatMSMU:
                  if (segment == 0) {
                        lineEdit()->setSelection(4, 3);
                        return true;
                        }
                  else if (segment == 1) {
                        lineEdit()->setSelection(7, 4);
                        return true;
                        }
                  else if (segment == 2) {
                        lineEdit()->setSelection(11, 3);
                        return true;
                        }
                break;

                case MusECore::TimeFormatNoOptions:
                case MusECore::TimeFormatAll:
                  //return true;
                break;
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
//           if(formatted())
//           {
// //             // FIXME: frames vs. ticks?
//             if (framesDisplay()) {
//               switch(segment) {
//                     case 0:  lineEdit()->setSelection(0,4); break;
//                     case 1:  lineEdit()->setSelection(4,3); break;
//                     case 2:  lineEdit()->setSelection(7,3); break;
//                     case 3:  lineEdit()->setSelection(10,2); break;
//                     }
//             }
//             else
//             {
//               switch(segment) {
//                     case 0:  lineEdit()->setSelection(0,5); break;
//                     case 1:  lineEdit()->setSelection(5,3); break;
//                     case 2:  lineEdit()->setSelection(8,3); break;
//                     }
//             }
//           }
//           else
//           {
//             lineEdit()->selectAll();
//           }
          
          
          switch(_timeFormatOptions)
          {
            case MusECore::TimeFormatFrames:
            case MusECore::TimeFormatTicks:
              lineEdit()->selectAll();
            break;

            case MusECore::TimeFormatBBT:
              switch(segment) {
                    case 0:  lineEdit()->setSelection(0,5); break;
                    case 1:  lineEdit()->setSelection(5,3); break;
                    case 2:  lineEdit()->setSelection(8,3); break;
                    }
            break;

            case MusECore::TimeFormatMSFS:
              switch(segment) {
                    case 0:  lineEdit()->setSelection(0,4); break;
                    case 1:  lineEdit()->setSelection(4,3); break;
                    case 2:  lineEdit()->setSelection(7,3); break;
                    case 3:  lineEdit()->setSelection(10,2); break;
                    }
            break;

            case MusECore::TimeFormatMSMU:
              switch(segment) {
                    case 0:  lineEdit()->setSelection(0,4); break;
                    case 1:  lineEdit()->setSelection(4,3); break;
                    case 2:  lineEdit()->setSelection(7,4); break;
                    case 3:  lineEdit()->setSelection(11,3); break;
                    }
            break;

            case MusECore::TimeFormatNoOptions:
            case MusECore::TimeFormatAll:
            break;
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

// //---------------------------------------------------------
// //   updateInputMask
// //---------------------------------------------------------
// 
// void PosEdit::updateInputMask()
// {
//   if(formatted())
//   {
// //     if (smpte())
//     if (framesDisplay())
//           lineEdit()->setInputMask("999:99:99:99;0");
//           //lineEdit()->setInputMask("999m99s99f99;0");
//     else
//           lineEdit()->setInputMask("9999.99.999;0");
//   }
//   else
//   {
//           //lineEdit()->setInputMask("999999999999;0");
// //           lineEdit()->setInputMask("000000000009");
//     // Ten digits plus unit indicator.
//     lineEdit()->setInputMask("0000000009X");
//   }
// }

//---------------------------------------------------------
//   updateInputMask
//---------------------------------------------------------

void PosEdit::updateInputMask()
{
  switch(_timeFormatOptions)
  {
    case MusECore::TimeFormatFrames:
    case MusECore::TimeFormatTicks:
      // Ten digits plus unit indicator.
      lineEdit()->setInputMask("0000000009X");
    break;

    case MusECore::TimeFormatBBT:
      lineEdit()->setInputMask("9999.99.999;0");
    break;

    case MusECore::TimeFormatMSFS:
      lineEdit()->setInputMask("999:99:99:99;0");
    break;

    case MusECore::TimeFormatMSMU:
      lineEdit()->setInputMask("999:99:999:999;0");
    break;

    case MusECore::TimeFormatNoOptions:
    case MusECore::TimeFormatAll:
      lineEdit()->setInputMask(QString());
    break;
  }
}

// //---------------------------------------------------------
// //   toggleTimeFormatOptions
// //---------------------------------------------------------
// 
// void PosEdit::toggleTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options, bool set)
// {
//   _timeFormatOptions.setOptions(options, set);
//   _pos.setType(smpte() ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
//   updateInputMask();
//   updateValue();
// }

// //---------------------------------------------------------
// //   setTimeFormatOptions
// //---------------------------------------------------------
// 
// void PosEdit::setTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options)
// {
//   _timeFormatOptions = options;
//   //_pos.setType(smpte() ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
//   updateInputMask();
//   updateValue();
// }

//---------------------------------------------------------
//   setTimeFormatOptions
//---------------------------------------------------------

void PosEdit::setTimeFormatOptions(const MusECore::TimeFormatOptions& options)
{
  _timeFormatOptions = options;
  //_pos.setType(smpte() ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
  updateInputMask();
  updateValue();
}

bool PosEdit::smpte() const
{
  return _pos.type() == MusECore::Pos::FRAMES;
}

// //---------------------------------------------------------
// //   setSmpte
// //---------------------------------------------------------
// 
// void PosEdit::setSmpte(bool f)
//       {
//       toggleTimeFormatOptions(MusECore::TimeModeFrames, f);
//       updateInputMask();
//       updateValue();
//       }

//---------------------------------------------------------
//   setSmpte
//---------------------------------------------------------

void PosEdit::setSmpte(bool f)
      {
      _pos.setType(f ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS);
      updateInputMask();
      updateValue();
      }

//---------------------------------------------------------
//   setType
//---------------------------------------------------------

void PosEdit::setType(const MusECore::Pos::TType& type)
{
  _pos.setType(type);
  updateInputMask();
  updateValue();
}
      
// //---------------------------------------------------------
// //   setFormatted
// //---------------------------------------------------------
// 
// void PosEdit::setFormatted(bool f)
// {
//   toggleTimeFormatOptions(MusECore::TimeFormatFormatted, f);
//   updateInputMask();
//   updateValue();
// }
// 
// void PosEdit::setFramesDisplay(bool f)
// {
//   toggleTimeFormatOptions(MusECore::TimeDisplayFrames, f);
//   updateInputMask();
//   updateValue();
// }

// //---------------------------------------------------------
// //   setValue
// //---------------------------------------------------------
// 
// void PosEdit::setValue(const MusECore::Pos& time)
// {
//       if(formatted() && _pos == time)
//       {
//         // Must check whether actual values dependent on tempo or sig changed...
// //         if (smpte()) {
//         if (framesDisplay()) {
//               int minute, sec, frame, subframe;
//               time.msf(&minute, &sec, &frame, &subframe);
//               if(minute != cur_minute || sec != cur_sec || frame != cur_frame || subframe != cur_subframe)
//                 updateValue();
//               }
//         else {
//               int bar, beat, tick;
//               time.mbt(&bar, &beat, &tick);
//               if(bar != cur_bar || beat != cur_beat || tick != cur_tick)
//                 updateValue();
//               }
//       }      
//       else
//       {
//         // We want to keep the user's desired type.
//         // Use this instead of assignment which sets everything.
//         //_pos.setPos(time);
//         _pos.setTickAndFrame(time);
//         updateValue();
//       }
// }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosEdit::setValue(const MusECore::Pos& time)
{
      // REMOVE Tim. clip. Added. Diagnostics.
      fprintf(stderr, "PosEdit::setValue(): %p\n", this);

//       if(formatted() && _pos == time)
//       {
//         // Must check whether actual values dependent on tempo or sig changed...
// //         if (smpte()) {
//         if (framesDisplay()) {
//               int minute, sec, frame, subframe;
//               time.msf(&minute, &sec, &frame, &subframe);
//               if(minute != cur_minute || sec != cur_sec || frame != cur_frame || subframe != cur_subframe)
//                 updateValue();
//               }
//         else {
//               int bar, beat, tick;
//               time.mbt(&bar, &beat, &tick);
//               if(bar != cur_bar || beat != cur_beat || tick != cur_tick)
//                 updateValue();
//               }
//       }      
//       else
//       {
//         // We want to keep the user's desired type.
//         // Use this instead of assignment which sets everything.
//         //_pos.setPos(time);
//         _pos.setTickAndFrame(time);
//         updateValue();
//       }
      
      
  switch(_timeFormatOptions)
  {
    case MusECore::TimeFormatNoOptions:
    case MusECore::TimeFormatFrames:
    case MusECore::TimeFormatTicks:
      // We want to keep the user's desired type.
      // Use this instead of assignment which sets everything.
      //_pos.setPos(time);
      _pos.setTickAndFrame(time);
      updateValue();
    break;

    case MusECore::TimeFormatBBT:
    {
      // Must check whether actual values dependent on tempo or sig changed...
      if(_pos == time)
      {
        int bar, beat, tick;
        time.mbt(&bar, &beat, &tick);
        if(bar != cur_bar || beat != cur_beat || tick != cur_tick)
          updateValue();
      }
      else
      {
        _pos.setTickAndFrame(time);
        updateValue();
      }
    }
    break;

    case MusECore::TimeFormatMSFS:
    {
      if(_pos == time)
      {
        int minute, sec, frame, subframe;
        time.msf(nullptr, &minute, &sec, &frame, &subframe);
        if(minute != cur_minute || sec != cur_sec || frame != cur_frame || subframe != cur_subframe)
          updateValue();
      }
      else
      {
        _pos.setTickAndFrame(time);
        updateValue();
      }
    }
    break;

    case MusECore::TimeFormatMSMU:
    {
      if(_pos == time)
      {
        int minute, sec, msec, usec;
        time.msmu(nullptr, &minute, &sec, &msec, &usec);
        if(minute != cur_msmu_minute || sec != cur_msmu_sec || msec != cur_msmu_msec || usec != cur_msmu_usec)
          updateValue();
      }
      else
      {
        _pos.setTickAndFrame(time);
        updateValue();
      }
    }
    break;

    case MusECore::TimeFormatAll:
    break;
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

// //---------------------------------------------------------
// //   updateValue
// //---------------------------------------------------------
// 
// void PosEdit::updateValue()
//       {
//       QString s;
//       const MusECore::Pos p(_pos);
//       const bool fr_disp = framesDisplay();
// 
//       if(formatted())
//       {
// //         if (smpte()) {
//         if (fr_disp) {
//               p.msf(&cur_minute, &cur_sec, &cur_frame, &cur_subframe);
//               s = QString("%1:%2:%3:%4")
//                   .arg(cur_minute,   3, 10, QLatin1Char('0'))
//                   .arg(cur_sec,      2, 10, QLatin1Char('0'))
//                   .arg(cur_frame,    2, 10, QLatin1Char('0'))
//                   .arg(cur_subframe, 2, 10, QLatin1Char('0'));
//               }
//         else {
//               p.mbt(&cur_bar, &cur_beat, &cur_tick);
//               s = QString("%1.%2.%3")
//                   .arg(cur_bar + 1,  4, 10, QLatin1Char('0'))
//                   .arg(cur_beat + 1, 2, 10, QLatin1Char('0'))
//                   .arg(cur_tick,     3, 10, QLatin1Char('0'));
//               }
//       }
//       else
//       {
//         //s = QString("%1").arg(_pos.posValue(), 12, 10, QLatin1Char('0'));
//         //s = QString("%1").arg(_pos.posValue(), 0, 10);
//         const MusECore::Pos::TType ttype = fr_disp ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS;
// //         s = QString("%1").arg(p.posValue(ttype), 12, 10);
//         s = QString("%1%2").arg(p.posValue(ttype), 10, 10).arg(fr_disp ? '.' : ':');
//       }
// 
// //       lineEdit()->clear();
// //       lineEdit()->end(false);
// //       lineEdit()->setModified(false);
//       lineEdit()->setText(s);
//       }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void PosEdit::updateValue()
      {
      // REMOVE Tim. clip. Added. Diagnostics.
      fprintf(stderr, "PosEdit::updateValue(): %p\n", this);

      QString s;
      const MusECore::Pos p(_pos);
//       const bool fr_disp = framesDisplay();
//       const MusECore::Pos::TType ttype = _pos.type();
//       const bool fr_disp = ttype == MusECore::Pos::FRAMES;

//       if(formatted())
//       {
// //         if (smpte()) {
//         if (fr_disp) {
//               p.msf(&cur_minute, &cur_sec, &cur_frame, &cur_subframe);
//               s = QString("%1:%2:%3:%4")
//                   .arg(cur_minute,   3, 10, QLatin1Char('0'))
//                   .arg(cur_sec,      2, 10, QLatin1Char('0'))
//                   .arg(cur_frame,    2, 10, QLatin1Char('0'))
//                   .arg(cur_subframe, 2, 10, QLatin1Char('0'));
//               }
//         else {
//               p.mbt(&cur_bar, &cur_beat, &cur_tick);
//               s = QString("%1.%2.%3")
//                   .arg(cur_bar + 1,  4, 10, QLatin1Char('0'))
//                   .arg(cur_beat + 1, 2, 10, QLatin1Char('0'))
//                   .arg(cur_tick,     3, 10, QLatin1Char('0'));
//               }
//       }
//       else
//       {
//         //s = QString("%1").arg(_pos.posValue(), 12, 10, QLatin1Char('0'));
//         //s = QString("%1").arg(_pos.posValue(), 0, 10);
//         const MusECore::Pos::TType ttype = fr_disp ? MusECore::Pos::FRAMES : MusECore::Pos::TICKS;
// //         s = QString("%1").arg(p.posValue(ttype), 12, 10);
//         s = QString("%1%2").arg(p.posValue(ttype), 10, 10).arg(fr_disp ? '.' : ':');
//       }
// 
      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatFrames:
          s = QString("%1%2").arg(p.posValue(MusECore::Pos::FRAMES), 10, 10).arg(':');
        break;

        case MusECore::TimeFormatTicks:
          s = QString("%1%2").arg(p.posValue(MusECore::Pos::TICKS), 10, 10).arg('.');
        break;

        case MusECore::TimeFormatBBT:
        {
          p.mbt(&cur_bar, &cur_beat, &cur_tick);
          s = QString("%1.%2.%3")
              .arg(cur_bar + 1,  4, 10, QLatin1Char('0'))
              .arg(cur_beat + 1, 2, 10, QLatin1Char('0'))
              .arg(cur_tick,     3, 10, QLatin1Char('0'));
        }
        break;

        case MusECore::TimeFormatMSFS:
        {
          p.msf(nullptr, &cur_minute, &cur_sec, &cur_frame, &cur_subframe);
          s = QString("%1:%2:%3:%4")
              .arg(cur_minute,   3, 10, QLatin1Char('0'))
              .arg(cur_sec,      2, 10, QLatin1Char('0'))
              .arg(cur_frame,    2, 10, QLatin1Char('0'))
              .arg(cur_subframe, 2, 10, QLatin1Char('0'));
        }
        break;

        case MusECore::TimeFormatMSMU:
        {
          p.msmu(nullptr, &cur_msmu_minute, &cur_msmu_sec, &cur_msmu_msec, &cur_msmu_usec);
          s = QString("%1:%2:%3:%4")
              .arg(cur_msmu_minute,   3, 10, QLatin1Char('0'))
              .arg(cur_msmu_sec,      2, 10, QLatin1Char('0'))
              .arg(cur_msmu_msec,     3, 10, QLatin1Char('0'))
              .arg(cur_msmu_usec,     3, 10, QLatin1Char('0'));
        }
        break;

        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
        break;
      }
      
      
      
      
//       lineEdit()->clear();
//       lineEdit()->end(false);
//       lineEdit()->setModified(false);
      blockSignals(true);
      lineEdit()->setText(s);
      blockSignals(false);
      }

//---------------------------------------------------------
//   stepEnables
//---------------------------------------------------------

QAbstractSpinBox::StepEnabled PosEdit::stepEnabled() const
      {
//       const bool frame_mode = smpte() && framesDisplay();
      int segment = curSegment();
      QAbstractSpinBox::StepEnabled en = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;

//       if(formatted())
//       {
// //         if (frame_mode) {
//         if (framesDisplay()) {
//               int minute, sec, frame, subframe;
//               _pos.msf(&minute, &sec, &frame, &subframe);
//               switch(segment) {
//                     case 0:
//                           if (minute == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           break;
//                     case 1:
//                           if (sec == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           else if (sec == 59)
//                                 en &= ~QAbstractSpinBox::StepUpEnabled;
//                           break;
//                     case 2:
//                           if (frame == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           else
//                           {
//                             int nf = 23;    // 24 frames sec
//                             switch(MusEGlobal::mtcType) {
//                                   //case 0:     // 24 frames sec
//                                   //      nf = 23;
//                                   //      break;
//                                   case 1:
//                                         nf = 24;  // 25 frames sec
//                                         break;
//                                   case 2:     // 30 drop frame
//                                   case 3:     // 30 non drop frame
//                                         nf = 29;
//                                         break;
//                                   default:
//                                         break;      
//                                   }
//                             //if (frame == 23)
//                             if (frame >= nf)
//                                 en &= ~QAbstractSpinBox::StepUpEnabled;
//                           }      
//                           break;
//                     case 3:
//                           if (subframe == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           else if (subframe == 99)
//                                 en &= ~QAbstractSpinBox::StepUpEnabled;
//                           break;
//                     }
//               }
//         else {
//               int bar, beat;
//               unsigned tick;
//               MusEGlobal::sigmap.tickValues(_pos.tick(), &bar, &beat, &tick);
//               unsigned tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
//               unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
//               int bm = tm / tb;
// 
//               switch (segment) {
//                     case 0:
//                           if (bar == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           break;
//                     case 1:
//                           if (beat == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           else {
//                                 if (beat >= (bm-1))
//                                       en &= ~QAbstractSpinBox::StepUpEnabled;
//                                 }
//                           break;
//                     case 2:
//                           if (tick == 0)
//                                 en &= ~QAbstractSpinBox::StepDownEnabled;
//                           else {
//                                 if (tick >= (tb-1))
//                                       en &= ~QAbstractSpinBox::StepUpEnabled;
//                                 }
//                           break;
//                     }
//               }
//       }
//       else
//       {
//         en = QAbstractSpinBox::StepNone;
//         if(_pos.posValue() > 0)
//           en = QAbstractSpinBox::StepDownEnabled;
//         if(_pos.posValue() < UINT_MAX)
//           en |= QAbstractSpinBox::StepUpEnabled;
//       }
//       
      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatFrames:
        case MusECore::TimeFormatTicks:
        case MusECore::TimeFormatMSFS:
        case MusECore::TimeFormatMSMU:
          en = QAbstractSpinBox::StepNone;
          if(_pos.posValue() > 0)
            en = QAbstractSpinBox::StepDownEnabled;
          if(_pos.posValue() < UINT_MAX)
            en |= QAbstractSpinBox::StepUpEnabled;
        break;

        case MusECore::TimeFormatBBT:
        {
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
        break;

//         case MusECore::TimeFormatMSFS:
//         {
//           int minute, sec, frame, subframe;
//           _pos.msf(&minute, &sec, &frame, &subframe);
//           switch(segment) {
//                 case 0:
//                       if (minute == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       break;
//                 case 1:
//                       if (sec == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else if (sec == 59)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;
//                       break;
//                 case 2:
//                       if (frame == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else
//                       {
//                         int nf = 23;    // 24 frames sec
//                         switch(MusEGlobal::mtcType) {
//                               //case 0:     // 24 frames sec
//                               //      nf = 23;
//                               //      break;
//                               case 1:
//                                     nf = 24;  // 25 frames sec
//                                     break;
//                               case 2:     // 30 drop frame
//                               case 3:     // 30 non drop frame
//                                     nf = 29;
//                                     break;
//                               default:
//                                     break;      
//                               }
//                         //if (frame == 23)
//                         if (frame >= nf)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;
//                       }      
//                       break;
//                 case 3:
//                       if (subframe == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else if (subframe == 99)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;
//                       break;
//                 }
//         }
//         break;

//         case MusECore::TimeFormatMSMU:
//         {
//           int minute, sec, msec, usec;
//           _pos.msmu(&minute, &sec, &msec, &usec);
//           switch(segment) {
//                 case 0:
//                       if (minute == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       break;
//                 case 1:
//                       if (sec == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else if (sec == 59)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;
//                       break;
//                 case 2:
//                       if (msec == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else if (sec == 999)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;      
//                       break;
//                 case 3:
//                       if (usec == 0)
//                             en &= ~QAbstractSpinBox::StepDownEnabled;
//                       else if (usec == 999)
//                             en &= ~QAbstractSpinBox::StepUpEnabled;
//                       break;
//                 }
//         }
//         break;

        case MusECore::TimeFormatAll:
        break;
      }
      
      
      
      
      return en;
      }

//---------------------------------------------------------
//   fixup
//---------------------------------------------------------

void PosEdit::fixup(QString& /*input*/) const
      {
      //fprintf(stderr, "fixup <%s>\n", input.toLatin1().constData()); 
      }

//---------------------------------------------------------
//   validate
//---------------------------------------------------------

QValidator::State PosEdit::validate(QString& s,int& /*i*/) const
{
      // REMOVE Tim. clip. Added. Diagnostics.
      //fprintf(stderr, "validate string:%s int:%d\n", s.toLatin1().data(), i);  
      fprintf(stderr, "validate string:%s\n", s.toLatin1().data());  

      QValidator::State state;
      QValidator::State rv = QValidator::Acceptable;
      // "By default, the pos parameter is not used by this [QIntValidator] validator."
      int dpos = 0;    
      
//       if(formatted())
//       {
// //         QStringList sl = s.split(smpte() ? ':' : '.');
//         QStringList sl = s.split(framesDisplay() ? ':' : '.');
// //         if (smpte()) 
//         if (framesDisplay()) 
//         {
//           if(sl.size() != 4)
//           {
//             fprintf(stderr, "validate smpte string:%s sections:%d != 4\n", s.toLatin1().data(), sl.size());  
//             return QValidator::Invalid;
//           }  
//           
//           validator->setRange(0, 999);
//           state = validator->validate(sl[0], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//             
//           validator->setRange(0, 59);
//           state = validator->validate(sl[1], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//             
//           int nf = 23;      // 24 frames sec
//           switch(MusEGlobal::mtcType) {
//                 //case 0:     // 24 frames sec
//                 //      nf = 23;
//                 //      break;
//                 case 1:
//                       nf = 24;  // 25 frames sec
//                       break;
//                 case 2:     // 30 drop frame
//                 case 3:     // 30 non drop frame
//                       nf = 29;
//                       break;
//                 default:
//                       break;      
//                 }
//           validator->setRange(0, nf);
//           state = validator->validate(sl[2], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//             
//           validator->setRange(0, 99);
//           state = validator->validate(sl[3], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//         }
//         else
//         {
//           if(sl.size() != 3)
//           {
//             fprintf(stderr, "validate bbt string:%s sections:%d != 3\n", s.toLatin1().data(), sl.size());  
//             return QValidator::Invalid;
//           }
//             
//           int tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
//           unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
//           if (tm==0)
//             return QValidator::Invalid;
//           int bm = tm / tb;
// 
//           validator->setRange(1, 9999);
//           //fprintf(stderr, "validate substring 0:%s\n", sl[0].toLatin1().data());  
//           // Special hack because validator says 0000 is intermediate.
//           if(sl[0] == "0000")
//             return QValidator::Invalid;
//           state = validator->validate(sl[0], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//             
//           validator->setRange(1, bm);
//           //fprintf(stderr, "validate substring 1:%s\n", sl[1].toLatin1().data());  
//           // Special hack because validator says 00 is intermediate.
//           if(sl[1] == "00")
//             return QValidator::Invalid;
//           state = validator->validate(sl[1], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//             
//           validator->setRange(0, tb-1);
//           //fprintf(stderr, "validate substring 2:%s\n", sl[2].toLatin1().data());  
//           state = validator->validate(sl[2], dpos);
//           if(state == QValidator::Invalid)
//             return state;
//           if(state == QValidator::Intermediate)
//             rv = state;
//         }
//       }
//       else
//       {
//         //// Normal input. Let the base do it.
//         //return QAbstractSpinBox::validate(s, i);
//         
//         // FIXME  We need unsigned int range! This limits the available range by half.
//         //validator->setRange(0, INT_MAX);
//         //state = validator->validate(s, dpos);
//         rv = QValidator::Acceptable;
//       }
//       
      
      
      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
        case MusECore::TimeFormatFrames:
        case MusECore::TimeFormatTicks:
          //// Normal input. Let the base do it.
          //return QAbstractSpinBox::validate(s, i);
          
          // FIXME  We need unsigned int range! This limits the available range by half.
          //validator->setRange(0, INT_MAX);
          //state = validator->validate(s, dpos);
          rv = QValidator::Acceptable;
        break;

        case MusECore::TimeFormatBBT:
        {
          QStringList sl = s.split('.');
          if(sl.size() != 3)
          {
            fprintf(stderr, "validate bbt string:%s sections:%d != 3\n", s.toLatin1().data(), sl.size());  
            return QValidator::Invalid;
          }
            
          int tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
          unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
          if (tm==0)
            return QValidator::Invalid;
          int bm = tm / tb;

          validator->setRange(1, 9999);
          //fprintf(stderr, "validate substring 0:%s\n", sl[0].toLatin1().data());  
          // Special hack because validator says 0000 is intermediate.
          if(sl[0] == "0000")
            return QValidator::Invalid;
          state = validator->validate(sl[0], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(1, bm);
          //fprintf(stderr, "validate substring 1:%s\n", sl[1].toLatin1().data());  
          // Special hack because validator says 00 is intermediate.
          if(sl[1] == "00")
            return QValidator::Invalid;
          state = validator->validate(sl[1], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(0, tb-1);
          //fprintf(stderr, "validate substring 2:%s\n", sl[2].toLatin1().data());  
          state = validator->validate(sl[2], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
        }
        break;

        case MusECore::TimeFormatMSFS:
        {
          QStringList sl = s.split(':');
          if(sl.size() != 4)
          {
            fprintf(stderr, "validate smpte string:%s sections:%d != 4\n", s.toLatin1().data(), sl.size());  
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
        break;

        case MusECore::TimeFormatMSMU:
        {
          QStringList sl = s.split(':');
          if(sl.size() != 4)
          {
            fprintf(stderr, "validate smpte string:%s sections:%d != 4\n", s.toLatin1().data(), sl.size());  
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

          validator->setRange(0, 999);
          state = validator->validate(sl[2], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
            
          validator->setRange(0, 999);
          state = validator->validate(sl[3], dpos);
          if(state == QValidator::Invalid)
            return state;
          if(state == QValidator::Intermediate)
            rv = state;
        }
        break;
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
      const bool has_sel = le->hasSelectedText();

      int segment = -1;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 100, 0))
      const int sel_end = le->selectionEnd(); // Qt 5.10
#else
      const int sel_start = le->selectionStart();
      const int sel_end = sel_start >= 0 ? sel_start + le->selectedText().size() : -1;
#endif

      // If text is selected it is a slightly different ballgame.
      // On doubleclick for example the line edit wants to put the cursor one PAST the end
      //  of the segment's selected text, which would put it into the next segment.
      if(has_sel && sel_end >= 0)
      {
//         if(formatted())
//         {
//   //         if (smpte()) {
//           if (framesDisplay()) {
//                 if (sel_end >= 0 && sel_end <= 4)
//                       segment = 0;
//                 else if (sel_end >= 5 && sel_end <= 7)
//                       segment = 1;
//                 else if (sel_end >= 8 && sel_end <= 10)
//                       segment = 2;
//                 else //if (sel_end >= 11)
//                       segment = 3;
//                 }
//           else {
//                 if (sel_end >= 0 && sel_end <= 5)
//                       segment = 0;
//                 else if (sel_end >= 6 && sel_end <= 8)
//                       segment = 1;
//                 else //if (sel_end >= 9)
//                       segment = 2;
//                 //else
//                 //      fprintf(stderr, "curSegment = -1, sel_end %d\n", sel_end);
//                 }
//         }
//         else
//         {
//           //if (sel_end >= 0 && sel_end <= 9)
//             segment = 0;
//           //else
//           //  segment = 1;
//         }
        
        switch(_timeFormatOptions)
        {
          case MusECore::TimeFormatNoOptions:
          case MusECore::TimeFormatAll:
          case MusECore::TimeFormatFrames:
          case MusECore::TimeFormatTicks:
            segment = 0;
          break;

          case MusECore::TimeFormatBBT:
          {
            if (sel_end >= 0 && sel_end <= 5)
                  segment = 0;
            else if (sel_end >= 6 && sel_end <= 8)
                  segment = 1;
            else //if (sel_end >= 9)
                  segment = 2;
            //else
            //      fprintf(stderr, "curSegment = -1, sel_end %d\n", sel_end);
          }
          break;

          case MusECore::TimeFormatMSFS:
          {
            if (sel_end >= 0 && sel_end <= 4)
                  segment = 0;
            else if (sel_end >= 5 && sel_end <= 7)
                  segment = 1;
            else if (sel_end >= 8 && sel_end <= 10)
                  segment = 2;
            else //if (sel_end >= 11)
                  segment = 3;
          }
          break;

          case MusECore::TimeFormatMSMU:
          {
            if (sel_end >= 0 && sel_end <= 4)
                  segment = 0;
            else if (sel_end >= 5 && sel_end <= 7)
                  segment = 1;
            else if (sel_end >= 8 && sel_end <= 11)
                  segment = 2;
            else //if (sel_end >= 12)
                  segment = 3;
          }
          break;
        }
      }
      else
      {
//         if(formatted())
//         {
//   //         if (smpte()) {
//           if (framesDisplay()) {
//                 if (pos >= 0 && pos <= 3)
//                       segment = 0;
//                 else if (pos >= 4 && pos <= 6)
//                       segment = 1;
//                 else if (pos >= 7 && pos <= 9)
//                       segment = 2;
//                 else //if (pos >= 10)
//                       segment = 3;
//                 }
//           else {
//                 if (pos >= 0 && pos <= 4)
//                       segment = 0;
//                 else if (pos >= 5 && pos <= 7)
//                       segment = 1;
//                 else //if (pos >= 8)
//                       segment = 2;
//                 //else
//                 //      fprintf(stderr, "curSegment = -1, pos %d\n", pos);
//                 }
//         }
//         else
//         {
//           //if (pos >= 0 && pos <= 9)
//             segment = 0;
//           //else
//           //  segment = 1;
//         }
        
        switch(_timeFormatOptions)
        {
          case MusECore::TimeFormatNoOptions:
          case MusECore::TimeFormatAll:
          case MusECore::TimeFormatFrames:
          case MusECore::TimeFormatTicks:
            segment = 0;
          break;

          case MusECore::TimeFormatBBT:
          {
            if (pos >= 0 && pos <= 4)
                  segment = 0;
            else if (pos >= 5 && pos <= 7)
                  segment = 1;
            else //if (pos >= 8)
                  segment = 2;
            //else
            //      fprintf(stderr, "curSegment = -1, pos %d\n", pos);
          }
          break;

          case MusECore::TimeFormatMSFS:
          {
            if (pos >= 0 && pos <= 3)
                  segment = 0;
            else if (pos >= 4 && pos <= 6)
                  segment = 1;
            else if (pos >= 7 && pos <= 9)
                  segment = 2;
            else //if (pos >= 10)
                  segment = 3;
          }
          break;

          case MusECore::TimeFormatMSMU:
          {
            if (pos >= 0 && pos <= 3)
                  segment = 0;
            else if (pos >= 4 && pos <= 6)
                  segment = 1;
            else if (pos >= 7 && pos <= 10)
                  segment = 2;
            else //if (pos >= 11)
                  segment = 3;
          }
          break;
        }
      }
      // REMOVE Tim. clip. Added. Diagnostics.
      //fprintf(stderr, "pos %d segment:%d\n", pos, segment);
      return segment;
      }

//---------------------------------------------------------
//   stepBy
//---------------------------------------------------------

void PosEdit::stepBy(int steps)
      {
      // REMOVE Tim. clip. Added. Diagnostics.
      //fprintf(stderr, "validate string:%s int:%d\n", s.toLatin1().data(), i);  
      fprintf(stderr, "stepBy steps:%d\n", steps);  

//       const bool frame_mode = smpte() && framesDisplay();
//       const MusECore::Pos::TType ttype = _pos.type();
//       const bool frame_mode = smpte() && framesDisplay();
//       const bool frame_mode = _pos.type() == MusECore::Pos::FRAMES;

//       const MusECore::Pos p(_pos);
      MusECore::Pos p(_pos);

      int segment = 0;
//       int selPos = 0;
//       int selLen = 0;

//       if(formatted())
//       //if(formatted() && ((smpte() && framesDisplay()) || (!smpte() && !framesDisplay())))
//       //if(formatted() && (smpte() || !framesDisplay()))
//       {
//         segment = curSegment();
//         bool changed = false;
// 
// //         if (smpte()) {
//         if (framesDisplay()) {
// //         if (frame_mode) {
//           
//             //if(smpte())
//             //{
//               int minute, sec, frame, subframe;
//               p.msf(&minute, &sec, &frame, &subframe);
//               switch(segment) {
//                     case 0:
//                           minute += steps;
//                           if (minute < 0)
//                                 minute = 0;
//                           break;
//                     case 1:
//                           sec += steps;
//                           if (sec < 0)
//                                 sec = 0;
//                           if (sec > 59)
//                                 sec = 59;
//                           break;
//                     case 2:
//                           {
//                             int nf = 23;      // 24 frames sec
//                             switch(MusEGlobal::mtcType) {
//                                   //case 0:     // 24 frames sec
//                                   //      nf = 23;
//                                   //      break;
//                                   case 1:
//                                         nf = 24;    // 25 frames sec
//                                         break;
//                                   case 2:     // 30 drop frame
//                                   case 3:     // 30 non drop frame
//                                         nf = 29;
//                                         break;
//                                   default:
//                                         break;      
//                                   }
//                             frame += steps;
//                             if (frame < 0)
//                                   frame = 0;
//                             //if (frame > 24)         //TD frame type?
//                             //      frame = 24;
//                             if (frame > nf)         
//                                   frame = nf;
//                           }
//                           break;
//                     case 3:
//                           subframe += steps;
//                           if (subframe < 0)
//                                 subframe = 0;
//                           if (subframe > 99)
//                                 subframe = 99;
//                           break;
//                     default:
//                           return;
//                     }
//               MusECore::Pos newPos(minute, sec, frame, subframe);
//               //unsigned int new_f = newPos.frame();
//               if(!smpte())
//               {
//                 newPos.setType(MusECore::Pos::TICKS);
//                 // Invalidate to get a new frame.
// //                 //newPos.invalidSn();
// //                 newPos.setType(MusECore::Pos::FRAMES);
//                 // Make sure there's at least enough for one tick increment.
//                 if(steps > 0 && newPos == _pos)
//                   newPos += 1;
//               }
//                 //new_f = MusECore::Pos(new_f, false);
//               if (!(newPos == _pos)) {
//                     changed = true;
//                     //_pos = newPos;
//                     _pos.setPos(newPos);
//                     }
//               }
//         else {
//               int bar, beat, tick;
//               p.mbt(&bar, &beat, &tick);
// 
//               int tb = MusEGlobal::sigmap.ticksBeat(p.tick());
//               //int tb = MusEGlobal::sigmap.ticksBeat(_pos.tick());
//               unsigned tm = MusEGlobal::sigmap.ticksMeasure(p.tick());
//               //unsigned tm = MusEGlobal::sigmap.ticksMeasure(_pos.tick());
//               int bm = tm / tb;
// 
//               switch(segment) {
//                     case 0:
//                           bar += steps;
//                           if (bar < 0)
//                                 bar = 0;
//                           break;
//                     case 1:
//                           beat += steps;
//                           if (beat < 0)
//                                 beat = 0;
//                           else if (beat >= bm)
//                                 beat = bm - 1;
//                           break;
//                     case 2:
//                           tick += steps;
//                           if (tick < 0)
//                                 tick = 0;
//                           else if (tick >= tb)
//                                 tick = tb -1;
//                           break;
//                     default:
//                           return;
//                     }
//               MusECore::Pos newPos(bar, beat, tick);
//               if (!(newPos == _pos)) {
//                     changed = true;
//                     //_pos = newPos;
//                     _pos.setPos(newPos);
//                     }
//               }
//               
//         if (changed) {
//               updateValue();
//               emit valueChanged(_pos);
//               }
//         //}
//       }
//       else
//       {
//         const unsigned int cur_val = frame_mode ? p.frame() : p.tick();
//         if(steps >= 0)
//         {
//           const unsigned int avail = UINT_MAX - cur_val;
//           if((unsigned int)steps > avail)
//             steps = avail;
//         }
//         else
//         {
//           if((unsigned int)-steps > cur_val)
//             steps = -cur_val;
//         }
//         const MusECore::Pos new_p(cur_val + steps, !frame_mode);
//         _pos.setPos(new_p);
//         updateValue();
//         emit valueChanged(_pos);
//       }

      

      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatFrames:
        {
//           const unsigned int cur_val = frame_mode ? p.frame() : p.tick();
//           const unsigned int cur_val = p.posValue();
          const unsigned int cur_val = p.frame();
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
//           const MusECore::Pos new_p(cur_val + steps, !frame_mode);
          // Construct a new position in FRAMES from the new frame value.
          const MusECore::Pos newPos(cur_val + steps, false);
          p.setPos(newPos);
          
          // If in TICKS mode, make sure there's at least enough for one tick increment.
          //if(!frame_mode && _pos == p)
          // Make sure there's at least enough for one increment.
          if(steps != 0 && _pos == p)
          {
            if(steps > 0)
              p = _pos + 1;
            else if(steps < 0)
              p = _pos - 1;
          }

          if(_pos != p) {
//             _pos.setPos(newPos);
            _pos.setPos(p);
            updateValue();
            emit valueChanged(_pos);
          }
        }
        break;

        case MusECore::TimeFormatTicks:
        {
//           const unsigned int cur_val = frame_mode ? p.frame() : p.tick();
          const unsigned int cur_val = p.tick();
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
//           const MusECore::Pos new_p(cur_val + steps, !frame_mode);
          // Construct a new position in TICKS from the new tick value.
          const MusECore::Pos newPos(cur_val + steps);
          p.setPos(newPos);
          // If in FRAMES mode, make sure there's at least enough for one frame increment.
          //if(frame_mode && _pos == p)
          // Make sure there's at least enough for one increment.
          if(steps != 0 && _pos == p)
          {
            if(steps > 0)
              p = _pos + 1;
            else if(steps < 0)
              p = _pos - 1;
          }
//           _pos.setPos(new_p);
          if(_pos != p) {
            _pos.setPos(p);
            updateValue();
            emit valueChanged(_pos);
          }
        }
        break;

        case MusECore::TimeFormatBBT:
        {
          segment = curSegment();
//           bool changed = false;
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
          // Construct a new position in TICKS from the new bbt value.
          MusECore::Pos newPos(bar, beat, tick);
          p.setPos(newPos);
          // If in FRAMES mode, make sure there's at least enough for one frame increment.
          //if(frame_mode && _pos == p)
          // Make sure there's at least enough for one increment.
          if(steps != 0 && _pos == p)
          {
            if(steps > 0)
              p = _pos + 1;
            else if(steps < 0)
              p = _pos - 1;
          }

//           if (newPos != _pos) {
          if (_pos != p) {
//                 changed = true;
                //_pos = newPos;
//                 _pos.setPos(newPos);
                _pos.setPos(p);
                updateValue();
                emit valueChanged(_pos);
                }
//           if (changed) {
//                 updateValue();
//                 emit valueChanged(_pos);
//                 }
        }
        break;

        case MusECore::TimeFormatMSFS:
        {
          segment = curSegment();
//           bool changed = false;
          int minute, sec, frame, subframe;
          p.msf(nullptr, &minute, &sec, &frame, &subframe);
//           MusECore::LargeIntRoundMode round_mode = steps > 0 ? MusECore::LargeIntRoundNearest : MusECore::LargeIntRoundDown;
          switch(segment) {
                case 0:
                      minute += steps;
//                       if (minute < 0)
//                             minute = 0;
                      break;
                case 1:
                      sec += steps;
//                       if (sec < 0)
//                             sec = 0;
//                       if (sec > 59)
//                             sec = 59;
                      break;
                case 2:
                      {
//                         int nf = 23;      // 24 frames sec
//                         switch(MusEGlobal::mtcType) {
//                               //case 0:     // 24 frames sec
//                               //      nf = 23;
//                               //      break;
//                               case 1:
//                                     nf = 24;    // 25 frames sec
//                                     break;
//                               case 2:     // 30 drop frame
//                               case 3:     // 30 non drop frame
//                                     nf = 29;
//                                     break;
//                               default:
//                                     break;      
//                               }
                        frame += steps;
//                         if (frame < 0)
//                               frame = 0;
//                         //if (frame > 24)         //TD frame type?
//                         //      frame = 24;
//                         if (frame > nf)         
//                               frame = nf;
                      }
                      break;
                case 3:
                      subframe += steps;
// //                       if (subframe < 0)
// //                             subframe = 0;
// //                       if (subframe > 99)
// //                             subframe = 99;
//                       if(steps > 0)
//                         round_mode = MusECore::LargeIntRoundUp;
                      break;
                default:
                      return;
                }

          // Construct a new position from the new msfs value. If the type is TICKS
          //  then set for ticks, and if we are counting upwards then set it to round up.
//           MusECore::Pos newPos(minute, sec, frame, subframe, !frame_mode, round_mode);

          // Construct a new position in FRAMES from the new msfs value.
          MusECore::Pos newPos(minute, sec, frame, subframe);

//           //unsigned int new_f = newPos.frame();
// //           if(!smpte())
//           if(!frame_mode)
//           {
//             newPos.setType(MusECore::Pos::TICKS);
//             // Invalidate to get a new frame.
// //                 //newPos.invalidSn();
// //                 newPos.setType(MusECore::Pos::FRAMES);
//             // Make sure there's at least enough for one tick increment.
//             if(steps > 0 && newPos == _pos)
//               newPos += 1;
//           }

          
          p.setPos(newPos);
          
          // If in TICKS mode, make sure there's at least enough for one tick increment.
          //if(!frame_mode && _pos == p)

          // Make sure there's at least enough for one increment.
          if(steps != 0 && _pos == p)
          {
            if(steps > 0)
              p = _pos + 1;
            else if(steps < 0)
              p = _pos - 1;
          }

          
            //new_f = MusECore::Pos(new_f, false);
//           if (!(newPos == _pos)) {
//           if(newPos != _pos) {
//           if(_pos != newPos) {
          if(_pos != p) {
//                 changed = true;
                //_pos = newPos;
//                 _pos.setPos(newPos);
                _pos.setPos(p);
                updateValue();
                emit valueChanged(_pos);
                }
        }
        break;

        case MusECore::TimeFormatMSMU:
        {
          segment = curSegment();
//           bool changed = false;
          int minute, sec, msec, usec;
          p.msmu(nullptr, &minute, &sec, &msec, &usec);
//           MusECore::LargeIntRoundMode round_mode = steps > 0 ? MusECore::LargeIntRoundNearest : MusECore::LargeIntRoundDown;
          switch(segment) {
                case 0:
                      minute += steps;
//                       if (minute < 0)
//                             minute = 0;
                      break;
                case 1:
                      sec += steps;
//                       if (sec < 0)
//                             sec = 0;
//                       if (sec > 59)
//                             sec = 59;
                      break;
                case 2:
                      msec += steps;
//                       if (msec < 0)
//                             msec = 0;
//                       if (msec > 999)
//                             msec = 999;
                      break;
                case 3:
                      usec += steps;
// //                       if (usec < 0)
// //                             usec = 0;
// //                       if (usec > 999)
// //                             usec = 999;
//                       if(steps > 0)
//                         round_mode = MusECore::LargeIntRoundUp;
                      break;
                default:
                      return;
                }

//           // Construct a new position from the new time. If the type is TICKS then set for ticks,
//           //  and if we are counting upwards then set it to round up.
//           MusECore::Pos newPos(0, minute, sec, msec, usec, !frame_mode, round_mode);

          // Construct a new position in FRAMES from the new time value.
          MusECore::Pos newPos(0, minute, sec, msec, usec, false);

          p.setPos(newPos);

          // If in TICKS mode, make sure there's at least enough for one tick increment.
          //if(!frame_mode && _pos == p)

          // Make sure there's at least enough for one increment.
          if(steps != 0 && _pos == p)
          {
            if(steps > 0)
              p = _pos + 1;
            else if(steps < 0)
              p = _pos - 1;
          }

          //unsigned int new_f = newPos.frame();
// //           if(!smpte())
//           if(!frame_mode)
//           {
//             newPos.setType(MusECore::Pos::TICKS);
//             // Invalidate to get a new frame.
// //                 //newPos.invalidSn();
// //                 newPos.setType(MusECore::Pos::FRAMES);
//             // Make sure there's at least enough for one tick increment.
//             if(steps > 0 && newPos == _pos)
//               newPos += 1;
//           }

//           if(!frame_mode)
//             newPos.setType(MusECore::Pos::TICKS);
          
            // Invalidate to get a new frame.
//                 //newPos.invalidSn();
//                 newPos.setType(MusECore::Pos::FRAMES);

//           // Make sure there's at least enough for one tick or frame increment.
//           if(steps > 0 && newPos <= _pos)
//           {
//             ++_pos;
//             updateValue();
//             emit valueChanged(_pos);
//           }
//           else
            //new_f = MusECore::Pos(new_f, false);
//           if (!(newPos == _pos)) {
//           if(newPos != _pos) {
          if(_pos != p) {
//                 changed = true;
                //_pos = newPos;
//                 _pos.setPos(newPos);
                _pos.setPos(p);
                updateValue();
                emit valueChanged(_pos);
                }
        }
        break;

        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
        break;
      }
    
      
      
      
//       if(formatted())
//       {
//         if(framesDisplay())
//         {
//           switch(segment) {
//                 case 0:
//                       selPos = 0;
//                       selLen = 4;
//                       break;
//                 case 1:
//                       selPos = 4;
//                       selLen = 3;
//                       break;
//                 case 2:
//                       {
//                         selPos = 7;
//                         selLen = 3;
//                       }
//                       break;
//                 case 3:
//                       selPos = 10;
//                       selLen = 2;
//                       break;
//                 }
//         } 
//         else
//         {
//           switch(segment) {
//                 case 0:
//                       selPos = 0;
//                       selLen = 5;
//                       break;
//                 case 1:
//                       selPos = 5;
//                       selLen = 3;
//                       break;
//                 case 2:
//                       selPos = 8;
//                       selLen = 3;
//                       break;
//                 }
//         }
//         lineEdit()->setSelection(selPos, selLen);
//       }
//       else
//       {
// //         lineEdit()->selectAll();
//         lineEdit()->setSelection(0, 10);
//       }

      
//       if(formatted())
//       {
// //             // FIXME: frames vs. ticks?
//         if (framesDisplay()) {
//           switch(segment) {
//                 case 0:  lineEdit()->setSelection(0,4); break;
//                 case 1:  lineEdit()->setSelection(4,3); break;
//                 case 2:  lineEdit()->setSelection(7,3); break;
//                 case 3:  lineEdit()->setSelection(10,2); break;
//                 }
//         }
//         else
//         {
//           switch(segment) {
//                 case 0:  lineEdit()->setSelection(0,5); break;
//                 case 1:  lineEdit()->setSelection(5,3); break;
//                 case 2:  lineEdit()->setSelection(8,3); break;
//                 }
//         }
//       }
//       else
//       {
//         lineEdit()->selectAll();
//       }
      
      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatFrames:
        case MusECore::TimeFormatTicks:
          lineEdit()->selectAll();
        break;

        case MusECore::TimeFormatBBT:
          switch(segment) {
                case 0:  lineEdit()->setSelection(0,5); break;
                case 1:  lineEdit()->setSelection(5,3); break;
                case 2:  lineEdit()->setSelection(8,3); break;
                }
        break;

        case MusECore::TimeFormatMSFS:
          switch(segment) {
                case 0:  lineEdit()->setSelection(0,4); break;
                case 1:  lineEdit()->setSelection(4,3); break;
                case 2:  lineEdit()->setSelection(7,3); break;
                case 3:  lineEdit()->setSelection(10,2); break;
                }
        break;

        case MusECore::TimeFormatMSMU:
          switch(segment) {
                case 0:  lineEdit()->setSelection(0,4); break;
                case 1:  lineEdit()->setSelection(4,3); break;
                case 2:  lineEdit()->setSelection(7,4); break;
                case 3:  lineEdit()->setSelection(11,3); break;
                }
        break;

        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
        break;
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

// //---------------------------------------------------------
// //   finishEdit
// //   Return true if position changed.
// //---------------------------------------------------------
// 
// bool PosEdit::finishEdit()
// {
//       bool ok;
//       bool changed = false;
//       if(formatted())
//       {
//         // If our validator did its job correctly, the entire line edit text should be valid now...
//         
// //         QStringList sl = text().split(smpte() ? ':' : '.');
//         QStringList sl = text().split(framesDisplay() ? ':' : '.');
// //         if (smpte()) 
//         if (framesDisplay()) 
//         {
//           if(sl.size() != 4)
//           {
//             fprintf(stderr, "finishEdit smpte string:%s sections:%d != 4\n", text().toLatin1().data(), sl.size());  
//             return false;
//           }  
//           
//           MusECore::Pos newPos(sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt());
//           if (!(newPos == _pos)) 
//           {
//             changed = true;
//             //_pos = newPos;
//             _pos.setPos(newPos);
//           }
//         }
//         else
//         {
//           if(sl.size() != 3)
//           {
//             fprintf(stderr, "finishEdit bbt string:%s sections:%d != 3\n", text().toLatin1().data(), sl.size());  
//             return false;
//           }
//             
//           MusECore::Pos newPos(sl[0].toInt() - 1, sl[1].toInt() - 1, sl[2].toInt());
//           if (!(newPos == _pos)) 
//           {
//             changed = true;
//             //_pos = newPos;
//             _pos.setPos(newPos);
//           }
//         }
//       }
//       else
//       {
//         // Make sure we leave enough for all the digits!
//         // 32 is not enough for this display. Also, plan for the future of 64 bit frame/tick etc.
//         const QString txt = text();
//         // Chopped undefined behaviour if string not long enough.
//         if(!txt.isEmpty())
//         {
//           // Discard (what should be) the unit character.
//           const unsigned long long new_val = txt.chopped(1).toULongLong(&ok);
// //           if(ok && new_val != _pos.posValue())
//           if(ok)
//           {
//             MusECore::Pos new_p(new_val, !framesDisplay());
//             if(!smpte())
//               new_p.setType(MusECore::Pos::TICKS);
//             if(new_p != _pos)
//             {
//               changed = true;
//   //             _pos.setPosValue(new_val);
//               _pos.setPos(new_p);
//             }
//           }
//         }
//       }
//   return changed;
// }

//---------------------------------------------------------
//   finishEdit
//   Return true if position changed.
//---------------------------------------------------------

bool PosEdit::finishEdit()
{
      bool ok;
      bool changed = false;
      const MusECore::Pos::TType ttype = _pos.type();
      const bool frame_mode = ttype == MusECore::Pos::FRAMES;

//       if(formatted())
//       {
//         // If our validator did its job correctly, the entire line edit text should be valid now...
//         
// //         QStringList sl = text().split(smpte() ? ':' : '.');
//         QStringList sl = text().split(framesDisplay() ? ':' : '.');
// //         if (smpte()) 
//         if (framesDisplay()) 
//         {
//           if(sl.size() != 4)
//           {
//             fprintf(stderr, "finishEdit smpte string:%s sections:%d != 4\n", text().toLatin1().data(), sl.size());  
//             return false;
//           }  
//           
//           MusECore::Pos newPos(sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt());
//           if (!(newPos == _pos)) 
//           {
//             changed = true;
//             //_pos = newPos;
//             _pos.setPos(newPos);
//           }
//         }
//         else
//         {
//           if(sl.size() != 3)
//           {
//             fprintf(stderr, "finishEdit bbt string:%s sections:%d != 3\n", text().toLatin1().data(), sl.size());  
//             return false;
//           }
//             
//           MusECore::Pos newPos(sl[0].toInt() - 1, sl[1].toInt() - 1, sl[2].toInt());
//           if (!(newPos == _pos)) 
//           {
//             changed = true;
//             //_pos = newPos;
//             _pos.setPos(newPos);
//           }
//         }
//       }
//       else
//       {
//         // Make sure we leave enough for all the digits!
//         // 32 is not enough for this display. Also, plan for the future of 64 bit frame/tick etc.
//         const QString txt = text();
//         // Chopped undefined behaviour if string not long enough.
//         if(!txt.isEmpty())
//         {
//           // Discard (what should be) the unit character.
//           const unsigned long long new_val = txt.chopped(1).toULongLong(&ok);
// //           if(ok && new_val != _pos.posValue())
//           if(ok)
//           {
//             MusECore::Pos new_p(new_val, !framesDisplay());
//             if(!smpte())
//               new_p.setType(MusECore::Pos::TICKS);
//             if(new_p != _pos)
//             {
//               changed = true;
//   //             _pos.setPosValue(new_val);
//               _pos.setPos(new_p);
//             }
//           }
//         }
//       }
      
      
      switch(_timeFormatOptions)
      {
        case MusECore::TimeFormatFrames:
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
              MusECore::Pos new_p(new_val, false);
              if(!frame_mode)
                new_p.setType(MusECore::Pos::TICKS);
              if(_pos != new_p)
              {
                changed = true;
    //             _pos.setPosValue(new_val);
                _pos.setPos(new_p);
              }
            }
          }
        }
        break;

        case MusECore::TimeFormatTicks:
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
              MusECore::Pos new_p(new_val, true);
              if(!frame_mode)
                new_p.setType(MusECore::Pos::TICKS);
              if(_pos != new_p)
              {
                changed = true;
    //             _pos.setPosValue(new_val);
                _pos.setPos(new_p);
              }
            }
          }
        }
        break;

        case MusECore::TimeFormatBBT:
        {
          //QStringList sl = text().split(framesDisplay() ? ':' : '.');
          const QStringList sl = text().split('.');
          if(sl.size() != 3)
          {
            fprintf(stderr, "finishEdit bbt string:%s sections:%d != 3\n", text().toLatin1().data(), sl.size());  
            return false;
          }
            
          // The 'mbt' constructor.
          MusECore::Pos newPos(sl[0].toInt() - 1, sl[1].toInt() - 1, sl[2].toInt());
          if(_pos != newPos) 
          {
            changed = true;
            //_pos = newPos;
            _pos.setPos(newPos);
          }
        }
        break;

        case MusECore::TimeFormatMSFS:
        {
          const QStringList sl = text().split(':');
          if(sl.size() != 4)
          {
            fprintf(stderr, "finishEdit smpte string:%s sections:%d != 4\n", text().toLatin1().data(), sl.size());  
            return false;
          }  
          
          // The 'msfs' constructor.
          MusECore::Pos newPos(sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt());
          if(_pos != newPos) 
          {
            changed = true;
            //_pos = newPos;
            _pos.setPos(newPos);
          }
        }
        break;

        case MusECore::TimeFormatMSMU:
        {
          const QStringList sl = text().split(':');
          if(sl.size() != 4)
          {
            fprintf(stderr, "finishEdit smpte string:%s sections:%d != 4\n", text().toLatin1().data(), sl.size());  
            return false;
          }  
          
          // The 'hmsms' constructor. No hours, just leave it blank.
          MusECore::Pos newPos(0, sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt(), false);
          if(_pos != newPos) 
          {
            changed = true;
            //_pos = newPos;
            _pos.setPos(newPos);
          }
        }
        break;

        case MusECore::TimeFormatNoOptions:
        case MusECore::TimeFormatAll:
        break;
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
  // REMOVE Tim. clip. Added. Diagnostics.
  fprintf(stderr, "PosEdit::contextMenuEvent\n");

  QMenu* menu = lineEdit()->createStandardContextMenu();
  if(!menu)
    return;

  event->accept();

  const QAbstractSpinBox::StepEnabled sen = stepEnabled();

  QAction* act = nullptr;

  act = menu->addAction(tr("Step up"));
  act->setData(ContextIdStepUp);
  act->setEnabled(sen & QAbstractSpinBox::StepUpEnabled);

  act = menu->addAction(tr("Step down"));
  act->setData(ContextIdStepDown);
  act->setEnabled(sen & QAbstractSpinBox::StepDownEnabled);

  menu->addSeparator();

  menu->addAction(new MenuTitleItem(tr("Mode:"), menu));

  if(_allowedTimeFormats & MusECore::TimeFormatFrames)
  {
    act = menu->addAction(tr("Frames"));
    act->setData(ContextIdFramesMode);
    act->setCheckable(true);
    act->setChecked(_timeFormatOptions == MusECore::TimeFormatFrames);
  }
  
  if(_allowedTimeFormats & MusECore::TimeFormatTicks)
  {
    act = menu->addAction(tr("Ticks"));
    act->setData(ContextIdTicksMode);
    act->setCheckable(true);
    act->setChecked(_timeFormatOptions == MusECore::TimeFormatTicks);
  }
  
  if(_allowedTimeFormats & MusECore::TimeFormatBBT)
  {
    act = menu->addAction(tr("Bar.Beat.Tick"));
    act->setData(ContextIdBBTMode);
    act->setCheckable(true);
    act->setChecked(_timeFormatOptions == MusECore::TimeFormatBBT);
  }
  
  if(_allowedTimeFormats & MusECore::TimeFormatMSFS)
  {
    act = menu->addAction(tr("Min:Sec:Fr:Subfr"));
    act->setData(ContextIdMSFSMode);
    act->setCheckable(true);
    act->setChecked(_timeFormatOptions == MusECore::TimeFormatMSFS);
  }
  
  if(_allowedTimeFormats & MusECore::TimeFormatMSMU)
  {
    act = menu->addAction(tr("Min:Sec:mSec:uSec"));
    act->setData(ContextIdMSMUMode);
    act->setCheckable(true);
    act->setChecked(_timeFormatOptions == MusECore::TimeFormatMSMU);
  }
  
//   if(_timeFormatOptions.flagsSet((MusECore::TimeFormatUserFormat | MusECore::TimeFormatUserFormat)))
//     menu->addSeparator();
//   
//   if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserMode))
//   {
//     act = menu->addAction(tr("Mode: Frames vs. ticks"));
//     act->setData(ContextIdMode);
//     act->setCheckable(true);
//     act->setChecked(smpte());
//   }
// 
//   if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserDisplayMode))
//   {
//     act = menu->addAction(tr("Display: Frames vs. ticks"));
//     act->setData(ContextIdDisplayMode);
//     act->setCheckable(true);
//     act->setChecked(framesDisplay());
//   }
// 
//   if(_timeFormatOptions.flagsSet(MusECore::TimeFormatUserFormat))
//   {
//     act = menu->addAction(tr("Format (as BBT or MSF)"));
//     act->setData(ContextIdFormat);
//     act->setCheckable(true);
//     act->setChecked(formatted());
//   }
  
//   menu->addSeparator();

//   act = menu->addAction(tr("Step up"));
//   act->setData(ContextIdStepUp);
//   act->setEnabled(sen & QAbstractSpinBox::StepUpEnabled);
// 
//   act = menu->addAction(tr("Step down"));
//   act->setData(ContextIdStepDown);
//   act->setEnabled(sen & QAbstractSpinBox::StepDownEnabled);
  
  act = menu->exec(event->globalPos());

  int idx = -1;
//   bool is_checked = false;
  if(act && act->data().isValid())
  {
    idx = act->data().toInt();
//     is_checked = act->isChecked();
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

//     case ContextIdMode:
//       toggleTimeFormatOptions(MusECore::TimeModeFrames, is_checked);
//     break;
// 
//     case ContextIdDisplayMode:
//       toggleTimeFormatOptions(MusECore::TimeDisplayFrames, is_checked);
//     break;
// 
//     case ContextIdFormat:
//       toggleTimeFormatOptions(MusECore::TimeFormatFormatted, is_checked);
//     break;

    case ContextIdFramesMode:
      if(_allowedTimeFormats & MusECore::TimeFormatFrames)
      {
        if(!_fixed_type)
          _pos.setType(MusECore::Pos::FRAMES);
        _timeFormatOptions = MusECore::TimeFormatFrames;
        updateInputMask();
        updateValue();
      }
    break;

    case ContextIdTicksMode:
      if(_allowedTimeFormats & MusECore::TimeFormatTicks)
      {
        if(!_fixed_type)
          _pos.setType(MusECore::Pos::TICKS);
        _timeFormatOptions = MusECore::TimeFormatTicks;
        updateInputMask();
        updateValue();
      }
    break;

    case ContextIdBBTMode:
      if(_allowedTimeFormats & MusECore::TimeFormatBBT)
      {
        if(!_fixed_type)
          _pos.setType(MusECore::Pos::TICKS);
        _timeFormatOptions = MusECore::TimeFormatBBT;
        updateInputMask();
        updateValue();
      }
    break;

    case ContextIdMSFSMode:
      if(_allowedTimeFormats & MusECore::TimeFormatMSFS)
      {
        if(!_fixed_type)
          _pos.setType(MusECore::Pos::FRAMES);
        _timeFormatOptions = MusECore::TimeFormatMSFS;
        updateInputMask();
        updateValue();
      }
    break;

    case ContextIdMSMUMode:
      if(_allowedTimeFormats & MusECore::TimeFormatMSMU)
      {
        if(!_fixed_type)
          _pos.setType(MusECore::Pos::FRAMES);
        _timeFormatOptions = MusECore::TimeFormatMSMU;
        updateInputMask();
        updateValue();
      }
    break;
  }
}

} // namespace MusEGui


