//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  meter_slider.h
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

#ifndef __COMPACT_PATCH_EDIT_H__
#define __COMPACT_PATCH_EDIT_H__

#include <QFrame>
#include "compact_slider.h"

class QMouseEvent;

namespace MusEGui {

class ElidedLabel;

//---------------------------------------------------------
//   CompactPatchEdit
//---------------------------------------------------------

class CompactPatchEdit : public QFrame
{
  Q_OBJECT

  private:
    int _maxAliasedPointSize;
    int _id;
    int _currentPatch;
    CompactSlider* _HBank;
    CompactSlider* _LBank;
    CompactSlider* _Prog;
    ElidedLabel* _patchNameLabel;
    
  private slots:
    void HBankValueStateChanged(double val, bool off, int id, int scrollMode);
    void LBankValueStateChanged(double val, bool off, int id, int scrollMode);
    void ProgValueStateChanged(double val, bool off, int id, int scrollMode);
    void HBankDoubleClicked(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void LBankDoubleClicked(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void ProgDoubleClicked(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void anySliderRightClicked(QPoint p, int id);
    void patchNamePressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    
  signals:
    void sliderRightClicked(QPoint p, int id);
    void patchNameClicked(QPoint p, int id);
    void patchNameRightClicked(QPoint p, int id);
    void valueChanged(double value, int id);
    // Both value and off state changed combined into one signal. 
    // Note the SliderBase::valueChanged signal is also available.
    void valueStateChanged(double value, bool off, int id, int scrollMode);

  public:
    CompactPatchEdit(QWidget *parent, const char *name = 0,
                Qt::Orientation orient = Qt::Vertical,
                CompactSlider::ScalePos scalePos = CompactSlider::None,
                QColor fillColor = QColor());
  
    static QSize getMinimumSizeHint(const QFontMetrics& fm, 
                                    Qt::Orientation orient = Qt::Vertical,
                                    CompactSlider::ScalePos scalePos = CompactSlider::None,
                                    int xMargin = 0, 
                                    int yMargin = 0);
    
    int id() const             { return _id; }
    void setId(int i)          { _id = i; }
    
    bool isOff() const;
    void setOff(bool v);
    
    double value() const;
    // Emits valueChanged and valueStateChanged signals if required.
    void setValue(double v) { setValueState(v, isOff()); }

    void setBorderColor(const QColor&);
    void setBarColor(const QColor&);
    void setSlotColor(const QColor&);
    void setThumbColor(const QColor&);
    
    // At what point size to switch from aliased text to non-aliased text. Zero means always use anti-aliasing. 
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label. 
    // If -1, no value has been set and default is each widget's setting.
    int maxAliasedPointSize() const { return _maxAliasedPointSize; }
    // Sets at what point size to switch from aliased text (brighter, crisper but may look too jagged and unreadable with some fonts) 
    //  to non-aliased text (dimmer, fuzzier but looks better). Zero means always use anti-aliasing. Default is each widget's setting.
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label. 
    void setMaxAliasedPointSize(int sz);
    
    // Both value and off state changed combined into one setter.
    // By default it is assumed that setting a value naturally implies resetting the 'off' state to false.
    // Emits valueChanged and valueStateChanged signals if required.
    void setValueState(double v, bool off = false);
    
    QString patchName() const;
    void setPatchName(const QString& patchName);
};

} // namespace MusEGui

#endif
