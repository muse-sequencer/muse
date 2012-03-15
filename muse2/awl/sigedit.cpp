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
#include "sigedit.h"
#include "al/sig.h"
//#include "sig.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>

namespace Awl {

      using AL::sigmap;

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

SigEdit::SigEdit(QWidget* parent)
   : QWidget(parent)
      {
      initialized = false;
      slash = new QLabel("/",this);
      zSpin = new SigSpinBox(this);
      nSpin = new SigSpinBox(this);
      zSpin->setFocusPolicy(Qt::StrongFocus);
      nSpin->setFocusPolicy(Qt::StrongFocus);
      zSpin->setRange(1,100);
      nSpin->setDenominator();
      nSpin->setRange(1,128);
      layout = new QHBoxLayout(this);
      layout->setContentsMargins(0,0,0,0);
      layout->setSpacing(1);
      layout->addWidget(zSpin);
      layout->addWidget(slash);
      layout->addWidget(nSpin);
      connect(zSpin, SIGNAL(valueChanged(int)), SLOT(setZ(int)));
      connect(nSpin, SIGNAL(valueChanged(int)), SLOT(setN(int)));
      connect(nSpin, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(zSpin, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(nSpin, SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(zSpin, SIGNAL(escapePressed()), SIGNAL(escapePressed()));

      connect(zSpin, SIGNAL(moveFocus()), SLOT(moveFocus()));
      connect(nSpin, SIGNAL(moveFocus()), SLOT(moveFocus()));
      zSpin->selectAll();
      }

SigEdit::~SigEdit()
      {
      delete layout;
      delete zSpin;
      delete nSpin;
      }

//---------------------------------------------------------
//   moveFocus
//---------------------------------------------------------

void SigEdit::moveFocus()
{
  if (zSpin->hasFocus()) {
    nSpin->setFocus();
    nSpin->selectAll();
  }
  else {
    zSpin->setFocus();
    zSpin->selectAll();
  }
}

//---------------------------------------------------------
//   setZ
//---------------------------------------------------------

void SigEdit::setZ(const int z)
{
  _sig.z=z;
  emit valueChanged(_sig);
}
//---------------------------------------------------------
//   setN
//---------------------------------------------------------

void SigEdit::setN(const int n)
{
  _sig.n=n;
  if (_sig.isValid()) {
    nSpin->setStyleSheet("");
    emit valueChanged(_sig);
  }
  else
    nSpin->setStyleSheet("QSpinBox { background-color: red; }");

}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void SigEdit::setValue(const AL::TimeSignature& s)
      {
      _sig = s;

      updateValue();
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void SigEdit::updateValue()
      {
      zSpin->setValue(_sig.z);
      nSpin->setValue(_sig.n);
      }

void SigEdit::paintEvent(QPaintEvent* event) {
            if (!initialized)
                  updateValue();
            initialized = true;
            QPainter p(this);
            p.fillRect(event->rect(), p.background());
            QWidget::paintEvent(event);
            }
void SigEdit::setFocus()
{
  zSpin->setFocus();
}

}

