//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __AWLCHECKBOX_H__
#define __AWLCHECKBOX_H__

namespace Awl {

//---------------------------------------------------------
//   CheckBox
//---------------------------------------------------------

class CheckBox : public QCheckBox {
      Q_OBJECT
      Q_PROPERTY(int id READ id WRITE setId)

      int _id;

   private slots:
      void hasToggled(bool val) {
            emit valueChanged(float(val), _id);
            }
   signals:
      void valueChanged(float, int);

   public slots:
      void setValue(float val) { setDown(val > 0.5f); }

   public:
      CheckBox(QWidget* parent);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

}

#endif

