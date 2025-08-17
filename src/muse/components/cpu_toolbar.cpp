//=========================================================
//  MusE
//  Linux Music Editor
//  cpu_toolbar.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QWidget>
#include <QString>
#include <QToolButton>
#include <QLatin1Char>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QApplication>

#include "cpu_toolbar.h"

namespace MusEGui
{

//---------------------------------
//   PaddedValueLabel
//---------------------------------

PaddedValueLabel::PaddedValueLabel(bool isFloat, QWidget* parent, Qt::WindowFlags f, 
                                   const QString& prefix, const QString& suffix)
    :QLabel(parent, f), _isFloat(isFloat), _prefix(prefix), _suffix(suffix)
{
    setObjectName("PaddedValueLabel");
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    _iVal = 0;
    _dVal = 0.0;
    _fieldWidth = 2;
    _precision = 1;
    updateText();
}

void PaddedValueLabel::setFieldWidth(int val)
{
    _fieldWidth = val;
    if(_fieldWidth < 0)
        _fieldWidth = 0;
    updateText();
}

void PaddedValueLabel::setPrecision(int val)
{
    _precision = val;
    if(_precision < 0)
        _precision = 0;
    updateText();
}

void PaddedValueLabel::setIntValue(int val)
{
    _iVal = val;
    updateText();
}

void PaddedValueLabel::setFloatValue(double val)
{
    _dVal = val;
    updateText();
}

void PaddedValueLabel::updateText()
{
    if(_isFloat)
        setText(QString("%1%L2%3").arg(_prefix).arg(_dVal, 0, 'f', _precision).arg(_suffix));
    else
        setText(QString("%1%2%3").arg(_prefix).arg(_iVal).arg(_suffix));
}

QSize PaddedValueLabel::sizeHint() const
{
    QString s;
    if(_isFloat)
        s = QString("%1%L2%3").arg(_prefix).arg(8.8888, _fieldWidth, 'f', _precision, QLatin1Char('8')).arg(_suffix);
    else
        s = QString("%1%2%3").arg(_prefix).arg(8, _fieldWidth, 10, QLatin1Char('8')).arg(_suffix);
    const int w = fontMetrics().horizontalAdvance(s);
    const int h = QLabel::sizeHint().height();
    return QSize(w, h);
}


XRunLabel::XRunLabel(QWidget* parent)
    : PaddedValueLabel(false, parent, Qt::Widget, "XRUNS: ")
{
    ensurePolished();
    defTextColor = palette().color(QPalette::WindowText).name();
}

void XRunLabel::updateText() {
    if (_iVal > 0)
        setStyleSheet("QLabel { color : red; }");
    else
        setStyleSheet("QLabel { color : " + defTextColor + "; }");

    PaddedValueLabel::updateText();
}

void XRunLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit doubleclicked();
}

//---------------------------------
//   CpuToolbar
//---------------------------------

CpuToolbar::CpuToolbar(QWidget* parent)
    : QToolBar(parent)
{
    init();
}

CpuToolbar::CpuToolbar(const QString& title, QWidget* parent)
    : QToolBar(title, parent)
{
    init();
}

void CpuToolbar::init()
{
    setObjectName("CpuLoadToolbar");

    _resetButton = new QToolButton(this);
    _resetButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _resetButton->setIcon(QIcon(":/svg/cpu.svg"));
    _resetButton->setObjectName("CpuLoadToolbarButton");
    _resetButton->setToolTip(tr("CPU load averaged over each GUI update period\nDSP load read from JACK\nNumber of xruns\n(click to reset)"));

    _cpuLabel = new PaddedValueLabel(true, this, Qt::Widget, "CPU: ", "%");
    _cpuLabel->setFieldWidth(5);
    _cpuLabel->setPrecision(1);
    _cpuLabel->setIndent(2);
    _dspLabel = new PaddedValueLabel(true, this, Qt::Widget, "DSP: ", "%");
    _dspLabel->setFieldWidth(5);
    _dspLabel->setPrecision(1);
    _xrunsLabel = new PaddedValueLabel(false, this, Qt::Widget, "XRUNS: ");
    _xrunsLabel->setFieldWidth(3);

    setValues(0.0f, 0.0f, 0);

    addWidget(_resetButton);
    addWidget(_cpuLabel);
    addWidget(_dspLabel);
    addWidget(_xrunsLabel);

    connect(_resetButton, SIGNAL(clicked(bool)), SIGNAL(resetClicked()));
}

void CpuToolbar::setValues(float cpuLoad, float dspLoad, long xRunsCount)
{
    _cpuLabel->setFloatValue(cpuLoad);
    _dspLabel->setFloatValue(dspLoad);
    _xrunsLabel->setIntValue(xRunsCount);
}

//---------------------------------
//   CpuStatusbar
//---------------------------------

CpuStatusBar::CpuStatusBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("CpuLoadStatusBar");
    setToolTip(tr("CPU load averaged over each GUI update period\nDSP load read from JACK\nNumber of xruns\n(click to reset)"));

    cpuLabel = new PaddedValueLabel(true, this, Qt::Widget, "CPU: ", "%");
    cpuLabel->setToolTip(tr("CPU load averaged over each GUI update period."));
    cpuLabel->setStatusTip(tr("CPU load averaged over each GUI update period."));
    cpuLabel->setFieldWidth(5);
    cpuLabel->setPrecision(1);

    dspLabel = new PaddedValueLabel(true, this, Qt::Widget, "DSP: ", "%");
    dspLabel->setToolTip(tr("DSP load read from JACK."));
    dspLabel->setStatusTip(tr("DSP load read from JACK."));
    dspLabel->setFieldWidth(5);
    dspLabel->setPrecision(1);

    xrunsLabel = new XRunLabel(this);
    xrunsLabel->setToolTip(tr("Number of xruns.\nDouble-click to reset."));
    xrunsLabel->setStatusTip(tr("Number of xruns.\nDouble-click to reset."));
    xrunsLabel->setFieldWidth(3);

    setValues(0.0f, 0.0f, 0);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(2);
    layout->addWidget(cpuLabel);
    layout->addWidget(dspLabel);
    layout->addWidget(xrunsLabel);

    connect(xrunsLabel, SIGNAL(doubleclicked()), SIGNAL(resetClicked()));
}

void CpuStatusBar::setValues(float cpuLoad, float dspLoad, long xRunsCount)
{
    cpuLabel->setFloatValue(cpuLoad);
    dspLabel->setFloatValue(dspLoad);
    xrunsLabel->setIntValue(xRunsCount);
}


}  // namespace MusEGui
