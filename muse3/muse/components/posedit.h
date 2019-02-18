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

#ifndef __POSEDIT_H__
#define __POSEDIT_H__

#include "pos.h"
#include "type_defs.h"

#include <QAbstractSpinBox>

namespace MusEGui {

//---------------------------------------------------------
//   PosEdit
//---------------------------------------------------------

class PosEdit : public QAbstractSpinBox
      {
      Q_OBJECT
      Q_PROPERTY(bool smpte READ smpte WRITE setSmpte)
      Q_PROPERTY(bool formatted READ formatted WRITE setFormatted)
      Q_PROPERTY(bool framesDisplay READ framesDisplay WRITE setFramesDisplay)

// REMOVE Tim. clip. Changed.
      // The time type and format, and whether the user can change them.
//       bool _formatted;
//       bool _smpte;
      MusECore::TimeFormatOptionsStruct _timeFormatOptions;
//       bool _enableUserFormat;
//       bool _enableUserType;

      MusECore::Pos _pos;
      bool initialized;
      bool _returnMode;
      int cur_minute, cur_sec, cur_frame, cur_subframe;
      int cur_bar, cur_beat, cur_tick;

      QIntValidator* validator;

      virtual void paintEvent(QPaintEvent* event);
      virtual void stepBy(int steps);
      virtual StepEnabled stepEnabled() const;
      virtual void fixup(QString& input) const;
      virtual QValidator::State validate(QString&, int&) const;
      int curSegment() const;
      virtual bool event(QEvent*);
      bool finishEdit();

   signals:
      void valueChanged(const MusECore::Pos&);

      // Choose these carefully, watch out for focusing recursion. 
      void returnPressed();
      void escapePressed();
      void lostFocus();      
      // This is emitted when focus lost or return pressed (same as QAbstractSpinBox). 
      void editingFinished();

//    private slots:
//       void customMenuRequested(QPoint pos);

   protected:
      enum ContextIds {
        ContextIdStepUp = 0x01,
        ContextIdStepDown = 0x02,
        ContextIdMode = 0x04,
        ContextIdDisplayMode = 0x08,
        ContextIdFormat = 0x10
      };

      void contextMenuEvent(QContextMenuEvent *event);
      void updateInputMask();

   public slots:
      void setValue(const MusECore::Pos& time);
      void setValue(int t);
      void setValue(const QString& s);

   public:
      PosEdit(QWidget* parent = 0,
        const MusECore::TimeFormatOptionsStruct& options = MusECore::TimeFormatOptionsStruct(
          MusECore::TimeFormatTicksFormatted | MusECore::TimeFormatUserAll));
      QSize sizeHint() const;

//       // Whether the user can change the format or time type.
//       bool isUserFormatEnabled() const { return _enableUserFormat; }
//       bool isUserTypeEnabled() const { return _enableUserType; }

      MusECore::TimeFormatOptionsStruct timeFormatOptions() const { return _timeFormatOptions; }
      void toggleTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options, bool set = true);
      void setTimeFormatOptions(const MusECore::TimeFormatOptionsStruct& options);

      MusECore::Pos pos() const { return _pos; }

      // Convenience and property methods.
      bool framesDisplay() const { return _timeFormatOptions.flagsSet(MusECore::TimeDisplayFrames); }
      void setFramesDisplay(bool f);
      bool formatted() const { return _timeFormatOptions.flagsSet(MusECore::TimeFormatFormatted); }
      void setFormatted(bool f);
      void setSmpte(bool);
      bool smpte() const { return _timeFormatOptions.flagsSet(MusECore::TimeModeFrames); }

      void setReturnMode(bool v) { _returnMode = v; } 
      bool returnMode() const    { return _returnMode; }
      void updateValue();
      };
}

#endif
