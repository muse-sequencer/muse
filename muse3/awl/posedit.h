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

///#include "al/pos.h"
#include "pos.h"

#include <QAbstractSpinBox>

namespace Awl {

      ///using AL::Pos;

//---------------------------------------------------------
//   PosEdit
//---------------------------------------------------------

class PosEdit : public QAbstractSpinBox
      {
      Q_OBJECT
      Q_PROPERTY(bool smpte READ smpte WRITE setSmpte)

      bool _smpte;
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

   public slots:
      void setValue(const MusECore::Pos& time);
      void setValue(int t);
      void setValue(const QString& s);

   public:
      PosEdit(QWidget* parent = 0);
      QSize sizeHint() const;

      MusECore::Pos pos() const { return _pos; }
      void setSmpte(bool);
      bool smpte() const { return _smpte; }
      void setReturnMode(bool v) { _returnMode = v; } 
      bool returnMode() const    { return _returnMode; }
      void updateValue();
      };
}

#endif
