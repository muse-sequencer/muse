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

#ifndef __SIGEDIT_H__
#define __SIGEDIT_H__

#include "al/sig.h"
//#include "sig.h"

#include <QAbstractSpinBox>

namespace AL {
      class TimeSignature;
      };

namespace Awl {

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

class SigEdit : public QAbstractSpinBox
      {
      Q_OBJECT

      AL::TimeSignature _sig;
      bool initialized;

      virtual void paintEvent(QPaintEvent* event);
      virtual void stepBy(int steps);
      virtual StepEnabled stepEnabled() const;
      virtual void fixup(QString& input) const;
      virtual QValidator::State validate(QString&, int&) const;
      void updateValue();
      int curSegment() const;
      virtual bool event(QEvent*);

   signals:
      void valueChanged(const AL::TimeSignature&);
      void returnPressed();

   public slots:
      void setValue(const AL::TimeSignature&);
      void setValue(const QString& s);

   public:
      SigEdit(QWidget* parent = 0);
      ~SigEdit();

      AL::TimeSignature sig() const { return _sig; }
      };
}

#endif
