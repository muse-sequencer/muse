//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  meter_slider.cpp
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
#include "compact_slider.h"
#include "elided_label.h"

namespace MusEGui {

CompactPatchEdit::CompactPatchEdit(QWidget *parent, const char *name,
               Qt::Orientation orient, CompactSlider::ScalePos scalePos, QColor fillColor)
            : QFrame(parent)

{
  setObjectName(name);

  _maxAliasedPointSize = -1;
  _id           = -1;
  _currentPatch = 0;
  
  _patchNameLabel = new ElidedLabel(0, Qt::ElideMiddle);
  _patchNameLabel->setObjectName("CompactPatchEditLabel");
  _patchNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
  
  _HBank = new CompactSlider(0, "CompactPatchEditHBank", orient, scalePos, tr("Hi"), QString(), QString(), QString(), fillColor);
  _LBank = new CompactSlider(0, "CompactPatchEditLBank", orient, scalePos, tr("Lo"), QString(), QString(), QString(), fillColor);
  _Prog = new CompactSlider(0, "CompactPatchEditProg", orient, scalePos, tr("Prg"), QString(), QString(), QString(), fillColor);

  _patchNameLabel->setToolTip(tr("Patch name"));
  _HBank->setToolTip(tr("Patch high-bank number\n(Ctrl-double-click on/off)"));
  _LBank->setToolTip(tr("Patch low-bank number\n(Ctrl-double-click on/off)"));
  _Prog->setToolTip(tr("Patch program\n(Ctrl-double-click on/off)"));
  
  _HBank->setHasOffMode(true);
  _LBank->setHasOffMode(true);
  _Prog->setHasOffMode(true);
  
  _HBank->setRange(0, 127, 1.0);
  _HBank->setValueDecimals(0);

  _LBank->setRange(0, 127, 1.0);
  _LBank->setValueDecimals(0);

  _Prog->setRange(0, 127, 1.0);
  _Prog->setValueDecimals(0);

  _patchNameLabel->setContentsMargins(0, 0, 0, 0);
  _HBank->setContentsMargins(0, 0, 0, 0);
  _LBank->setContentsMargins(0, 0, 0, 0);
  _Prog->setContentsMargins(0, 0, 0, 0);
  
  _HBank->setMargins(1, 1);
  _LBank->setMargins(1, 1);
  _Prog->setMargins(1, 1);
  
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  
  layout->addWidget(_patchNameLabel);
  layout->addWidget(_HBank);
  layout->addWidget(_LBank);
  layout->addWidget(_Prog);
  
  connect(_patchNameLabel, SIGNAL(pressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), SLOT(patchNamePressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  
  connect(_HBank, SIGNAL(valueStateChanged(double,bool,int,int)), 
                  SLOT(HBankValueStateChanged(double,bool,int,int)));
  connect(_LBank, SIGNAL(valueStateChanged(double,bool,int,int)), 
                  SLOT(LBankValueStateChanged(double,bool,int,int)));
  connect(_Prog,  SIGNAL(valueStateChanged(double,bool,int,int)), 
                  SLOT(ProgValueStateChanged(double,bool,int,int)));
  
  connect(_HBank, SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(HBankDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  connect(_LBank, SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(LBankDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  connect(_Prog,  SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(ProgDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));

  connect(_HBank, SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
  connect(_LBank, SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
  connect(_Prog,  SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
}

// Static.      
QSize CompactPatchEdit::getMinimumSizeHint(const QFontMetrics& fm,
                                        Qt::Orientation orient,
                                        CompactSlider::ScalePos scalePos,
                                        int xMargin,
                                        int yMargin)
{
  const QSize ctrl_sz = CompactSlider::getMinimumSizeHint(fm, orient, scalePos, xMargin, yMargin);
  
  // HACK Try to find the size of a label
//   QLabel* dummyLabel = new QLabel("WWW", this);
//   dummyLabel->setMargin(yMargin);
//   const QSize lbl_sz = dummyLabel->sizeHint();
  const int lbl_h = fm.height() + 2 * yMargin;
  
  //const int h = 3 * ctrl_sz.height() + lbl_sz.height();
  const int h = 3 * ctrl_sz.height() + lbl_h;
  switch(orient) {
        case Qt::Vertical:
              return QSize(16, h);
              break;
        case Qt::Horizontal:
              return QSize(16, h);
              break;
        }
  return QSize(10, 10);
}
      
bool CompactPatchEdit::isOff() const
{
  return _Prog->isOff();
}

void CompactPatchEdit::setMaxAliasedPointSize(int sz)
{ 
  _maxAliasedPointSize = sz;
  _HBank->setMaxAliasedPointSize(sz);
  _LBank->setMaxAliasedPointSize(sz);
  _Prog->setMaxAliasedPointSize(sz);
}

void CompactPatchEdit::setOff(bool v)
{
  _HBank->setOff(v);
  _LBank->setOff(v);
  _Prog->setOff(v);
}

double CompactPatchEdit::value() const
{
  return _currentPatch;
}


void CompactPatchEdit::setValueState(double v, bool off)
{
  _currentPatch = int(v);
  const int hb = (_currentPatch >> 16) & 0xff;
  const int lb = (_currentPatch >> 8) & 0xff;
  const int pr = _currentPatch & 0xff;

  if(pr == 0xff)
    off = true;
  
  if(hb == 0xff)
    _HBank->setOff(true);
  else
    _HBank->setValueState(hb, off);
    
  if(lb == 0xff)
    _LBank->setOff(true);
  else
    _LBank->setValueState(lb, off);
  
  if(pr == 0xff)
    _Prog->setOff(true);
  else
    _Prog->setValueState(pr, off);
}

QString CompactPatchEdit::patchName() const 
{ 
  return _patchNameLabel->text(); 
}

void CompactPatchEdit::setPatchName(const QString& patchName) 
{ 
  _patchNameLabel->setText(patchName); 
}

void CompactPatchEdit::HBankValueStateChanged(double val, bool off, int /*id*/, int scrollMode)
{
  const int hb = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xffff) | (off ? 0xff0000 : (hb << 16));
  
  if(isOff())
  {
    _LBank->blockSignals(true);
    _LBank->setOff(false);
    _LBank->blockSignals(false);
    
    _Prog->blockSignals(true);
    _Prog->setOff(false);
    _Prog->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id, scrollMode);
}

void CompactPatchEdit::LBankValueStateChanged(double val, bool off, int /*id*/, int scrollMode)
{
  const int lb = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xff00ff) | (off ? 0xff00 : (lb << 8));

  if(isOff())
  {
    _HBank->blockSignals(true);
    _HBank->setOff(false);
    _HBank->blockSignals(false);
    
    _Prog->blockSignals(true);
    _Prog->setOff(false);
    _Prog->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id, scrollMode);
}

void CompactPatchEdit::ProgValueStateChanged(double val, bool off, int /*id*/, int scrollMode)
{
  const int pr = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xffff00) | pr;
  
  if(off)
  {
    _HBank->blockSignals(true);
    _HBank->setOff(off);
    _HBank->blockSignals(false);
    
    _LBank->blockSignals(true);
    _LBank->setOff(off);
    _LBank->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id, scrollMode);
}

void CompactPatchEdit::HBankDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::LBankDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::ProgDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::anySliderRightClicked(QPoint p, int /*id*/)
{
  emit sliderRightClicked(p, _id);
}

void CompactPatchEdit::patchNamePressed(QPoint p, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers /*keys*/)
{
  if(buttons == Qt::LeftButton)
    emit patchNameClicked(p, _id);
  else if(buttons == Qt::RightButton)
    emit patchNameRightClicked(mapToGlobal(p), _id);
}


} // namespace MusEGui
