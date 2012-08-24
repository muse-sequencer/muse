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
#include <awl/sigspinbox.h>

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

namespace AL {
      class TimeSignature;
      };

namespace Awl {

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

class SigEdit : public QWidget
      {
      Q_OBJECT
      
      AL::TimeSignature _sig;
      bool initialized;
      QLabel *slash;
      SigSpinBox *zSpin;
      SigSpinBox *nSpin;
      QHBoxLayout *layout;

      virtual void paintEvent(QPaintEvent* event);
      void updateValue();

   signals:
      void valueChanged(const AL::TimeSignature&);
      void returnPressed();
      void escapePressed();

   private slots:
      void setN(const int n);
      void setZ(const int z);
      void moveFocus();

   public slots:
      void setValue(const AL::TimeSignature&);
      void setFocus();

   public:
      SigEdit(QWidget* parent = 0);
      ~SigEdit();
      AL::TimeSignature sig() const { return _sig; }
      void setFrame(bool v) { zSpin->setFrame(v); nSpin->setFrame(v); }
      };
}

#endif
