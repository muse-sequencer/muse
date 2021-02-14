//=========================================================
//  MusE
//  Linux Music Editor
//
//  popup_double_spinbox.h
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __POPUP_DOUBLE_SPINBOX_H__
#define __POPUP_DOUBLE_SPINBOX_H__

#include <QDoubleSpinBox>

class QEvent;

namespace MusEGui {

//---------------------------------------------------------
//   PopupDoubleSpinBox
//---------------------------------------------------------

class PopupDoubleSpinBox : public QDoubleSpinBox {
  Q_OBJECT

  private:
    bool _closePending;

  protected:
    virtual bool event(QEvent*);

  signals:
    void returnPressed();
    void escapePressed();

  public:
    PopupDoubleSpinBox(QWidget* parent=0);
};

} // namespace MusEGui

#endif
