//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.cpp,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include "pitchedit.h"
#include "mrconfig.h"
#include "globals.h"

#include <QCloseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   MRConfig
//    Midi Remote Control Config
//---------------------------------------------------------

MRConfig::MRConfig(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    b1->setChecked(MusEGlobal::rcEnable);
    sbStop->setValue(MusEGlobal::rcStopNote);
    sbRecord->setValue(MusEGlobal::rcRecordNote);
    sbGotoLeftMark->setValue(MusEGlobal::rcGotoLeftMarkNote);
    sbPlay->setValue(MusEGlobal::rcPlayNote);
    sbForward->setValue(MusEGlobal::rcForwardNote);
    sbRewind->setValue(MusEGlobal::rcBackwardNote);
    steprec_box->setValue(MusEGlobal::rcSteprecNote);

    b2->setChecked(MusEGlobal::rcEnableCC);
    sbPlayCC->setValue(MusEGlobal::rcPlayCC);
    sbStopCC->setValue(MusEGlobal::rcStopCC);
    sbRecCC->setValue(MusEGlobal::rcRecordCC);
    sbGotoLeftMarkCC->setValue(MusEGlobal::rcGotoLeftMarkCC);
//    sbInsertRestCC->setValue(MusEGlobal::rcInsertPauseCC);
    sbForwardCC->setValue(MusEGlobal::rcForwardCC);
    sbBackwardCC->setValue(MusEGlobal::rcBackwardCC);

    connect(b1, &QCheckBox::toggled, this, &MRConfig::setRcEnable);
    connect(sbStop, &PitchEdit::editingFinished, this, &MRConfig::setRcStopNote);
    connect(sbRecord, &PitchEdit::editingFinished, this, &MRConfig::setRcRecordNote);
    connect(sbGotoLeftMark, &PitchEdit::editingFinished, this, &MRConfig::setRcGotoLeftMarkNote);
    connect(sbPlay, &PitchEdit::editingFinished, this, &MRConfig::setRcPlayNote);
    connect(sbForward, &PitchEdit::editingFinished, this, &MRConfig::setRcForwardNote);
    connect(sbRewind, &PitchEdit::editingFinished, this, &MRConfig::setRcBackwardNote);
    connect(steprec_box, &PitchEdit::editingFinished, this, &MRConfig::setRcSteprecNote);

    connect(b2, &QCheckBox::toggled, this, &MRConfig::setRcEnableCC);
    connect(sbPlayCC, &QSpinBox::editingFinished, this, &MRConfig::setRcPlayCC);
    connect(sbStopCC, &QSpinBox::editingFinished, this, &MRConfig::setRcStopCC);
    connect(sbRecCC, &QSpinBox::editingFinished, this, &MRConfig::setRcRecordCC);
    connect(sbGotoLeftMarkCC, &QSpinBox::editingFinished, this, &MRConfig::setRcGotoLeftMarkCC);
    connect(sbForwardCC, &QSpinBox::editingFinished, this, &MRConfig::setRcForwardCC);
    connect(sbBackwardCC, &QSpinBox::editingFinished, this, &MRConfig::setRcBackwardCC);
//    connect(sbInsertRestCC, &QSpinBox::editingFinished, this, &MRConfig::setRcInsertRestCC);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MRConfig::closeEvent(QCloseEvent* ev)
{
    emit hideWindow();
    QWidget::closeEvent(ev);
}

void MRConfig::setRcEnable(bool f)
{
    MusEGlobal::rcEnable = f;
}

void MRConfig::setRcPlayNote()
{
    MusEGlobal::rcPlayNote = static_cast<unsigned char>(sbPlay->value());
}

void MRConfig::setRcStopNote()
{
    MusEGlobal::rcStopNote = static_cast<unsigned char>(sbStop->value());
}

void MRConfig::setRcRecordNote()
{
    MusEGlobal::rcRecordNote = static_cast<unsigned char>(sbRecord->value());
}

void MRConfig::setRcGotoLeftMarkNote()
{
    MusEGlobal::rcGotoLeftMarkNote = static_cast<unsigned char>(sbGotoLeftMark->value());
}

void MRConfig::setRcSteprecNote()
{
    MusEGlobal::rcSteprecNote = static_cast<unsigned char>(steprec_box->value());
}

void MRConfig::setRcForwardNote()
{
    MusEGlobal::rcForwardNote = static_cast<unsigned char>(sbForward->value());
}

void MRConfig::setRcBackwardNote()
{
    MusEGlobal::rcBackwardNote = static_cast<unsigned char>(sbRewind->value());
}


void MRConfig::setRcEnableCC(bool b)
{
    MusEGlobal::rcEnableCC = b;
}

void MRConfig::setRcPlayCC()
{
    MusEGlobal::rcPlayCC = static_cast<unsigned char>(sbPlayCC->value());
}

void MRConfig::setRcStopCC()
{
    MusEGlobal::rcStopCC = static_cast<unsigned char>(sbStopCC->value());
}

void MRConfig::setRcRecordCC()
{
    MusEGlobal::rcRecordCC = static_cast<unsigned char>(sbRecCC->value());
}

void MRConfig::setRcGotoLeftMarkCC()
{
    MusEGlobal::rcGotoLeftMarkCC = static_cast<unsigned char>(sbGotoLeftMarkCC->value());
}

//void MRConfig::setRcInsertRestCC()
//{
//    MusEGlobal::rcInsertPauseCC = static_cast<unsigned char>(sbInsertRestCC->value());
//}

void MRConfig::setRcForwardCC()
{
    MusEGlobal::rcForwardCC = static_cast<unsigned char>(sbForwardCC->value());
}

void MRConfig::setRcBackwardCC()
{
    MusEGlobal::rcBackwardCC = static_cast<unsigned char>(sbBackwardCC->value());
}



} // namespace MusEGui
