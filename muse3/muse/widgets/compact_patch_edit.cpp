//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_patch_edit.cpp
//  (C) Copyright 2015-2016 Tim E. Real (terminator356 on sourceforge)
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

#include <QVBoxLayout>
#include <QMouseEvent>

#include "compact_patch_edit.h"
#include "lcd_widgets.h"
#include "elided_label.h"

namespace MusEGui {

CompactPatchEdit::CompactPatchEdit(QWidget *parent,
                                   const char *name,
                                   QColor /*fillColor*/)
            : QFrame(parent)

{
  setObjectName(name);

  _orient = ReadoutHorizontal;
  _showPatchLabel = true;

  _maxAliasedPointSize = -1;
  _id           = -1;
  _currentPatch = 0;

  _patchNameLabel = new ElidedLabel(0, Qt::ElideNone);
  _patchNameLabel->setObjectName("CompactPatchEditLabel");
  _patchNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
  _patchNameLabel->setHasOffMode(true);

  _patchEdit = new LCDPatchEdit();

  _patchNameLabel->setToolTip(tr("Patch name"));

  _patchNameLabel->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(_patchNameLabel);
  layout->addWidget(_patchEdit);

  connect(_patchNameLabel, SIGNAL(pressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)),
          SLOT(patchNamePressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  connect(_patchNameLabel, SIGNAL(returnPressed(QPoint,int,Qt::KeyboardModifiers)),
          SLOT(patchNameReturnPressed(QPoint,int,Qt::KeyboardModifiers)));

  connect(_patchEdit, SIGNAL(valueChanged(int,int)),
          SLOT(patchEditValueChanged(int,int)));

  connect(_patchEdit, SIGNAL(rightClicked(QPoint,int)),
          SLOT(patchEditRightClicked(QPoint,int)));
}

CompactPatchEdit::~CompactPatchEdit()
{
  if(_patchEdit)
    delete _patchEdit;
}

// Static.
QSize CompactPatchEdit::getMinimumSizeHint(const QFontMetrics& fm,
                                        Qt::Orientation orient,
                                        int xMargin,
                                        int yMargin)
{
  const QSize ctrl_sz = LCDPatchEdit::getMinimumSizeHint(
    fm,
    xMargin,
    yMargin,
    orient == Qt::Horizontal ? LCDPatchEdit::PatchHorizontal : LCDPatchEdit::PatchVertical
  );

  // HACK Try to find the size of a label
//   QLabel* dummyLabel = new QLabel("WWW", this);
//   dummyLabel->setMargin(yMargin);
//   const QSize lbl_sz = dummyLabel->sizeHint();
  const int lbl_h = fm.height() + 2 * yMargin;

  const int h = ctrl_sz.height() + lbl_h;

  switch(orient) {
        case Qt::Horizontal:
              return QSize(ctrl_sz.width(), h);   // Patch edit is dominant.
              break;
        case Qt::Vertical:
              return QSize(16, h);
              break;
        }
  return QSize(10, 10);
}

// void CompactPatchEdit::keyPressEvent(QKeyEvent* e)
// {
//   switch (e->key())
//   {
//     case Qt::Key_Escape:
//     break;
//
//     default:
//     break;
//   }
//
//   e->ignore();
//   return QFrame::keyPressEvent(e);
// }

void CompactPatchEdit::setReadoutColor(const QColor& c)
{
  _patchEdit->setReadoutColor(c);
   //update();
}

void CompactPatchEdit::setReadoutOrientation(ReadoutOrientation orient)
{
  _orient = orient;
}

void CompactPatchEdit::setShowPatchLabel(bool v)
{
  _showPatchLabel = v;
}

void CompactPatchEdit::setMaxAliasedPointSize(int sz)
{
  _maxAliasedPointSize = sz;
  _patchEdit->setMaxAliasedPointSize(sz);
}

// QSize CompactPatchEdit::sizeHint() const
// {
//   return getMinimumSizeHint(fontMetrics(),
//                             _orient == ReadoutHorizontal ? Qt::Horizontal : Qt::Vertical,
//                             frameWidth(), //_xMargin,
//                             frameWidth() //_yMargin
//                            );
// }

int CompactPatchEdit::value() const
{
  return _currentPatch;
}

void CompactPatchEdit::setValue(int v)
{
  if(_currentPatch != v)
  {
    _currentPatch = v;
    //update();
  }
  _patchEdit->setValue(v);
}

void CompactPatchEdit::setLastValidValue(int v)
{
  _patchEdit->setLastValidPatch(v);
}

void CompactPatchEdit::setLastValidBytes(int hbank, int lbank, int prog)
{
  _patchEdit->setLastValidBytes(hbank, lbank, prog);
}

QString CompactPatchEdit::patchName() const
{
  return _patchNameLabel->text();
}

void CompactPatchEdit::setPatchName(const QString& patchName)
{
  _patchNameLabel->setText(patchName);
}

void CompactPatchEdit::setPatchNameOff(bool v)
{
  _patchNameLabel->setOff(v);
}

void CompactPatchEdit::patchEditValueChanged(int val, int /*id*/)
{
  _currentPatch = val;
  emit valueChanged(_currentPatch, _id);
}

void CompactPatchEdit::patchEditDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {

  }
}

void CompactPatchEdit::patchEditRightClicked(QPoint p, int /*id*/)
{
  emit patchValueRightClicked(p, _id);
}

void CompactPatchEdit::patchNamePressed(QPoint p, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers /*keys*/)
{
  if(buttons == Qt::LeftButton)
    emit patchNameClicked(p, _id);
  else if(buttons == Qt::RightButton)
    emit patchNameRightClicked(mapToGlobal(p), _id);
}

void CompactPatchEdit::patchNameReturnPressed(QPoint p, int /*id*/, Qt::KeyboardModifiers /*keys*/)
{
  emit patchNameClicked(p, _id);
}


} // namespace MusEGui
