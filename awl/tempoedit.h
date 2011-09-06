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

#ifndef __AWLTEMPOEDIT_H__
#define __AWLTEMPOEDIT_H__

#include <QDoubleSpinBox>

namespace Awl {

//---------------------------------------------------------
//   TempoEdit
//---------------------------------------------------------

class TempoEdit : public QDoubleSpinBox {
      Q_OBJECT

      double curVal;

   protected:
      QSize sizeHint() const;

   private slots:
      void newValue(double);

   public slots:
      void setTempo(int);

   signals:
   	void tempoChanged(int);

   public:
      TempoEdit(QWidget*);
      int tempo() const;
      };

}

#endif

