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

#ifndef __AWLCOMBOBOX_H__
#define __AWLCOMBOBOX_H__

#include <QComboBox>

namespace Awl {

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QComboBox {
      Q_OBJECT
      Q_PROPERTY(int id READ id WRITE setId)

      int _id;

   private slots:
      void activatedIntern(int val) {
            emit valueChanged(double(val), _id);
            }

   signals:
      void valueChanged(double, int);

   public slots:
      void setValue(double val) { setCurrentIndex(int(val)); }

   public:
      ComboBox(QWidget* parent);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

}

#endif
